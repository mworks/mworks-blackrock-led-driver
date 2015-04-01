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


void Device::describeComponent(ComponentInfo &info) {
    IODevice::describeComponent(info);
    
    info.setSignature("iodevice/blackrock_led_driver");
    
    info.addParameter(RUNNING, false);
    info.addParameter(TEMP_A, false);
    info.addParameter(TEMP_B, false);
    info.addParameter(TEMP_C, false);
    info.addParameter(TEMP_D, false);
}


Device::Device(const ParameterValueMap &parameters) :
    IODevice(parameters),
    handle(nullptr),
    intensityChanged(true),
    filePlaying(false),
    lastRunDuration(0)
{
    if (!(parameters[RUNNING].empty())) {
        running = VariablePtr(parameters[RUNNING]);
    }
    if (!(parameters[TEMP_A].empty())) {
        tempA = VariablePtr(parameters[TEMP_A]);
    }
    if (!(parameters[TEMP_B].empty())) {
        tempB = VariablePtr(parameters[TEMP_B]);
    }
    if (!(parameters[TEMP_C].empty())) {
        tempC = VariablePtr(parameters[TEMP_C]);
    }
    if (!(parameters[TEMP_D].empty())) {
        tempD = VariablePtr(parameters[TEMP_D]);
    }
    
    intensity.fill(WordValue::zero());
}


Device::~Device() {
    lock_guard lock(mutex);
    
    if (checkStatusTask) {
        checkStatusTask->cancel();
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
    
    boost::weak_ptr<Device> weakThis(component_shared_from_this<Device>());
    checkStatusTask = Scheduler::instance()->scheduleUS(FILELINE,
                                                        0,
                                                        200000,  // Check status every 200ms
                                                        M_REPEAT_INDEFINITELY,
                                                        [weakThis]() {
                                                            if (auto sharedThis = weakThis.lock()) {
                                                                lock_guard lock(sharedThis->mutex);
                                                                sharedThis->checkStatus();
                                                            }
                                                            return nullptr;
                                                        },
                                                        M_DEFAULT_IODEVICE_PRIORITY,
                                                        M_DEFAULT_IODEVICE_WARN_SLOP_US,
                                                        M_DEFAULT_IODEVICE_FAIL_SLOP_US,
                                                        M_MISSED_EXECUTION_DROP);
    
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


void Device::run(MWTime duration) {
    lock_guard lock(mutex);
    
    if (!checkIfFileStopped()) {
        return;
    }
    
    if (filePlaying) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "LED driver is already running");
        return;
    }
    
    if (intensityChanged || (duration != lastRunDuration)) {
        WORD period;
        std::size_t samplesUsed;
        if (!(quantizeDuration(duration, period, samplesUsed) &&
              setFileTimePeriod(period) &&
              loadFile(samplesUsed)))
        {
            return;
        }
        intensityChanged = false;
        lastRunDuration = duration;
    }
    
    if (!startFilePlaying()) {
        return;
    }
    
    filePlaying = true;
    if (running && !running->getValue().getBool()) {
        running->setValue(true);
    }
}


bool Device::quantizeDuration(MWTime duration, WORD &period, std::size_t &samplesUsed) {
    if (duration < minDuration || duration > maxDuration) {
        merror(M_IODEVICE_MESSAGE_DOMAIN,
               "LED driver run duration must be between %lld us and %g s",
               minDuration,
               double(maxDuration) / 1e6);
        return false;
    }
    
    if (duration % periodIncrement != 0) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "LED driver run duration must be a multiple of %lld us", periodIncrement);
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
    
    merror(M_IODEVICE_MESSAGE_DOMAIN, "Requested run duration (%lld us) is not compatible with LED driver", duration);
    return false;
}


bool Device::setFileTimePeriod(WORD period) {
    SetFileTimePeriodMessage msg;
    
    msg.getBody().period = period;
    
    if (!perform(msg)) {
        return false;
    }
    
    if (msg.getBody().period != period) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "LED driver responded with incorrect period");
        return false;
    }
    
    return true;
}


bool Device::loadFile(std::size_t samplesUsed) {
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
    
    return true;
}


bool Device::startFilePlaying() {
    StartFilePlayingRequest request;
    StartFilePlayingResponse response;
    
    if (!perform(request, response)) {
        return false;
    }
    
    if (!response.getBody().filePlayStarted) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "LED driver failed to start file play");
        return false;
    }
    
    return true;
}


void Device::checkStatus() {
    if (!checkStatusTask) {
        // We've already been canceled, so don't try to perform any I/O
        return;
    }
    
    if (!checkIfFileStopped()) {
        return;
    }
    
    ThermistorValuesRequest request;
    ThermistorValuesResponse response;
    
    if (!perform(request, response)) {
        return;
    }
    
#ifndef MW_BLACKROCK_LEDDRIVER_DEBUG
    announceTemp(tempA, response.getBody().tempA);
    announceTemp(tempB, response.getBody().tempB);
    announceTemp(tempC, response.getBody().tempC);
    announceTemp(tempD, response.getBody().tempD);
#endif
}


bool Device::checkIfFileStopped() {
    if (filePlaying) {
        IsFilePlayingRequest request;
        IsFilePlayingResponse response;
        
        if (!perform(request, response)) {
            return false;
        }
        
        if (!response.getBody().filePlaying) {
            filePlaying = false;
            if (running && running->getValue().getBool()) {
                running->setValue(false);
            }
        }
    }
    
    return true;
}


inline void Device::announceTemp(VariablePtr &var, WORD value) {
    if (var) {
        var->setValue(double(value) / 1000.0);
    }
}


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER


























