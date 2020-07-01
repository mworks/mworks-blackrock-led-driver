//
//  BlackrockLEDDriverDevice.cpp
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 9/24/14.
//  Copyright (c) 2014 The MWorks Project. All rights reserved.
//

#include "BlackrockLEDDriverDevice.h"


BEGIN_NAMESPACE_MW_BLACKROCK_LEDDRIVER


const std::string Device::RUNNING("running");
const std::string Device::TEMP_A("temp_a");
const std::string Device::TEMP_B("temp_b");
const std::string Device::TEMP_C("temp_c");
const std::string Device::TEMP_D("temp_d");
const std::string Device::TEMP_CALC("temp_calc");
const std::string Device::SIMULATE_DEVICE("simulate_device");


void Device::describeComponent(ComponentInfo &info) {
    IODevice::describeComponent(info);
    
    info.setSignature("iodevice/blackrock_led_driver");
    
    info.addParameter(RUNNING, false);
    info.addParameter(TEMP_A, false);
    info.addParameter(TEMP_B, false);
    info.addParameter(TEMP_C, false);
    info.addParameter(TEMP_D, false);
    info.addParameter(TEMP_CALC, "none");
    info.addParameter(SIMULATE_DEVICE, "NO");
}


Device::Device(const ParameterValueMap &parameters) :
    IODevice(parameters),
    running(optionalVariable(parameters[RUNNING])),
    tempA(optionalVariable(parameters[TEMP_A])),
    tempB(optionalVariable(parameters[TEMP_B])),
    tempC(optionalVariable(parameters[TEMP_C])),
    tempD(optionalVariable(parameters[TEMP_D])),
    tempCalc(variableOrText(parameters[TEMP_CALC])),
    simulateDevice(parameters[SIMULATE_DEVICE]),
    clock(Clock::instance()),
    handle(nullptr),
    intensityChanged(true),
    filePlaying(false),
    lastRunDuration(0),
    simulatedRunStartTime(0)
{
    intensity.fill(WordValue::zero());
}


Device::~Device() {
    lock_guard lock(mutex);
    
    if (checkStatusTask) {
        checkStatusTask->cancel();
    }
    
    if (handle || simulateDevice) {
        stopFilePlaying();
    }
    
    if (handle) {
        FT_STATUS status = FT_Close(handle);
        if (FT_OK != status) {
            merror(M_IODEVICE_MESSAGE_DOMAIN, "Cannot close LED driver (status: %d)", status);
        }
    }
}


bool Device::initialize() {
    lock_guard lock(mutex);
    
    if (simulateDevice) {
        mwarning(M_IODEVICE_MESSAGE_DOMAIN, "LED driver simulation is enabled");
    } else {
        FT_STATUS status;
        
        if (FT_OK != (status = FT_OpenEx(const_cast<char *>("Blinky 1.0"), FT_OPEN_BY_DESCRIPTION, &handle))) {
            switch (status) {
                case FT_DEVICE_NOT_FOUND:
                    merror(M_IODEVICE_MESSAGE_DOMAIN, "LED driver was not found. Is the USB cable connected?");
                    break;
                    
                case FT_DEVICE_NOT_OPENED:
                    merror(M_IODEVICE_MESSAGE_DOMAIN,
                           "LED driver was found but could not be opened. This is probably due to a conflict "
                           "with a system device driver. To resolve this issue, open the Terminal application "
                           "and execute the following command:\n\n\t"
                           "sudo kextunload -b com.apple.driver.AppleUSBFTDI\n");
                    break;
                    
                default:
                    merror(M_IODEVICE_MESSAGE_DOMAIN, "Cannot open LED driver (status: %d)", status);
                    break;
            }
            return false;
        }
        
        // Set read timeout to 2s, write timeout to 1s
        if (FT_OK != (status = FT_SetTimeouts(handle, 2000, 1000))) {
            merror(M_IODEVICE_MESSAGE_DOMAIN, "Cannot set LED driver I/O timeouts (status: %d)", status);
            return false;
        }
    }
    
    boost::weak_ptr<Device> weakThis(component_shared_from_this<Device>());
    checkStatusTask = Scheduler::instance()->scheduleUS(FILELINE,
                                                        0,
                                                        200000,  // Check status every 200ms
                                                        M_REPEAT_INDEFINITELY,
                                                        [weakThis]() {
                                                            if (auto sharedThis = weakThis.lock()) {
                                                                lock_guard lock(sharedThis->mutex);
                                                                sharedThis->checkIfFileStopped();
                                                            }
                                                            return nullptr;
                                                        },
                                                        M_DEFAULT_IODEVICE_PRIORITY,
                                                        M_DEFAULT_IODEVICE_WARN_SLOP_US,
                                                        M_DEFAULT_IODEVICE_FAIL_SLOP_US,
                                                        M_MISSED_EXECUTION_DROP);
    
    return true;
}


bool Device::stopDeviceIO() {
    lock_guard lock(mutex);
    stopFilePlaying();
    return true;
}


void Device::setIntensity(const std::set<int> &channels, double value) {
    lock_guard lock(mutex);
    
    if (value < 0.0 || value > 1.0) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "LED driver channel intensity must be between 0 and 1");
        return;
    }
    
    WORD wordValue = std::round(value * double(std::numeric_limits<WORD>::max()));
    
    for (int channelNum : channels) {
        if ((channelNum < 1) || (channelNum > intensity.size())) {
            merror(M_IODEVICE_MESSAGE_DOMAIN, "Invalid LED driver channel number: %d", channelNum);
        } else {
            intensity[channelNum - 1] = wordValue;
        }
    }
    
    intensityChanged = true;
}


void Device::prepare(MWTime duration) {
    lock_guard lock(mutex);
    updateFile(duration);
}


void Device::run(MWTime duration) {
    lock_guard lock(mutex);
    if (updateFile(duration)) {
        startFilePlaying();
    }
}


void Device::stop() {
    lock_guard lock(mutex);
    stopFilePlaying();
}


static void announceTemp(const VariablePtr &var, WORD rawValue, double pullup) {
    if (var) {
        auto value = double(rawValue);
        
        if (pullup == 0.0) {
            // For compatibility with old firmware that pre-calculated temperature and sent it
            // in millidegrees Celsius
            value /= 1000.0;
        } else {
            // Calculate resistance value of thermistor in kΩ with a voltage divider with the
            // specified pullup resistance
            value = pullup / ((double(0xFFFF) / value) - 1.0);
            
            // Calculate temperature in Celsius (linear fit)
            value = -4.4617 * value + 66.0;
        }
        
        var->setValue(value);
    }
}


void Device::readTemps() {
    lock_guard lock(mutex);
    
    auto currentTempCalc = tempCalc->getValue().getString();
    boost::algorithm::to_lower(currentTempCalc);
    double pullup = 0.0;
    
    if (currentTempCalc == "5k") {
        pullup = 5.0;
    } else if (currentTempCalc == "10k") {
        pullup = 10.0;
    } else if (currentTempCalc != "none") {
        merror(M_IODEVICE_MESSAGE_DOMAIN,
               "LED driver temperature calculation type (\"%s\") is invalid; using \"none\" instead",
               currentTempCalc.c_str());
    }
    
    if (!simulateDevice) {
        ThermistorValuesRequest request;
        ThermistorValuesResponse response;
        
        if (!perform(request, response)) {
            return;
        }
        
        announceTemp(tempA, response.getBody().tempA, pullup);
        announceTemp(tempB, response.getBody().tempB, pullup);
        announceTemp(tempC, response.getBody().tempC, pullup);
        announceTemp(tempD, response.getBody().tempD, pullup);
    }
}


bool Device::updateFile(MWTime duration) {
    if (!checkIfFileStopped()) {
        return false;
    }
    
    if (filePlaying) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "LED driver is already running");
        return false;
    }
    
    if (intensityChanged || (duration != lastRunDuration)) {
        WORD period;
        std::size_t samplesUsed;
        
        if (!(quantizeDuration(duration, period, samplesUsed) &&
              setFileTimePeriod(period) &&
              loadFile(samplesUsed)))
        {
            return false;
        }
        
        intensityChanged = false;
        lastRunDuration = duration;
        
        if (samplesUsed < numSamples) {
            mwarning(M_IODEVICE_MESSAGE_DOMAIN,
                     "LED driver run duration (%g ms) requires %g ms of padding after end of exposure "
                     "(all LEDs will be off during this interval)",
                     double(duration) / 1e3,
                     double((numSamples - samplesUsed) * period * periodIncrement) / 1e3);
        }
    }
    
    return true;
}


bool Device::quantizeDuration(MWTime duration, WORD &period, std::size_t &samplesUsed) {
    if (duration < minDuration || duration > maxDuration) {
        merror(M_IODEVICE_MESSAGE_DOMAIN,
               "LED driver run duration must be between %g ms and %g s",
               double(minDuration) / 1e3,
               double(maxDuration) / 1e6);
        return false;
    }
    
    if (duration % periodIncrement != 0) {
        merror(M_IODEVICE_MESSAGE_DOMAIN,
               "LED driver run duration must be a multiple of %g ms",
               double(periodIncrement) / 1e3);
        return false;
    }
    
    const MWTime normDuration = duration / periodIncrement;
    const MWTime normPeriodMin = MWTime(std::ceil(double(normDuration) / double(numSamples)));
    const MWTime normPeriodMax = std::min(normDuration, maxDuration / periodIncrement);
    
    for (MWTime normPeriod = normPeriodMin; normPeriod <= normPeriodMax; normPeriod++) {
        if (normDuration % normPeriod == 0) {
            period = normPeriod;
            samplesUsed = normDuration / normPeriod;
            return true;
        }
    }
    
    merror(M_IODEVICE_MESSAGE_DOMAIN,
           "Requested run duration (%g ms) is not compatible with LED driver",
           double(duration) / 1e3);
    return false;
}


bool Device::setFileTimePeriod(WORD period) {
    if (!simulateDevice) {
        SetFileTimePeriodMessage msg;
        
        msg.getBody().period = period;
        
        if (!perform(msg)) {
            return false;
        }
        
        if (msg.getBody().period != period) {
            merror(M_IODEVICE_MESSAGE_DOMAIN, "LED driver responded with incorrect period");
            return false;
        }
    }
    
    return true;
}


bool Device::loadFile(std::size_t samplesUsed) {
    if (!simulateDevice) {
        LoadFileRequest request;
        auto &samples = request.getBody().samples;
        
        for (std::size_t i = 0; i < samples.size(); i++) {
            if (i < samplesUsed) {
                samples[i] = intensity;
            } else {
                samples[i].fill(WordValue::zero());
            }
        }
        
        LoadFileResponse response;
        
        if (!perform(request, response)) {
            return false;
        }
        
        if (!response.getBody().fileLoaded) {
            merror(M_IODEVICE_MESSAGE_DOMAIN, "LED driver failed to load file");
            return false;
        }
    }
    
    return true;
}


bool Device::startFilePlaying() {
    if (simulateDevice) {
        simulatedRunStartTime = clock->getCurrentTimeUS();
    } else {
        StartFilePlayingRequest request;
        StartFilePlayingResponse response;
        
        if (!perform(request, response)) {
            return false;
        }
        
        if (!response.getBody().filePlaying) {
            merror(M_IODEVICE_MESSAGE_DOMAIN, "LED driver failed to start file play");
            return false;
        }
    }
    
    filePlaying = true;
    if (running && !running->getValue().getBool()) {
        running->setValue(true);
    }
    
    return true;
}


bool Device::checkIfFileStopped() {
    if (filePlaying) {
        bool fileStopped = false;
        
        if (simulateDevice) {
            fileStopped = (clock->getCurrentTimeUS() - simulatedRunStartTime) >= lastRunDuration;
        } else {
            IsFilePlayingRequest request;
            IsFilePlayingResponse response;
            
            if (!perform(request, response)) {
                return false;
            }
            
            fileStopped = !response.getBody().filePlaying;
        }
        
        if (fileStopped) {
            filePlaying = false;
            if (running && running->getValue().getBool()) {
                running->setValue(false);
            }
        }
    }
    
    return true;
}


bool Device::stopFilePlaying() {
    if (filePlaying) {
        if (!simulateDevice) {
            StopFilePlayingRequest request;
            StopFilePlayingResponse response;
            
            if (!perform(request, response)) {
                return false;
            }
            
            if (response.getBody().filePlaying) {
                merror(M_IODEVICE_MESSAGE_DOMAIN, "LED driver failed to stop file play");
                return false;
            }
        }
        
        filePlaying = false;
        if (running && running->getValue().getBool()) {
            running->setValue(false);
        }
    }
    
    return true;
}


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER
