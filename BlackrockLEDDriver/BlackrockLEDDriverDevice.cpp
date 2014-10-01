//
//  BlackrockLEDDriverDevice.cpp
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 9/24/14.
//  Copyright (c) 2014 The MWorks Project. All rights reserved.
//

#include "BlackrockLEDDriverDevice.h"


BEGIN_NAMESPACE_MW_BLACKROCK_LEDDRIVER


const std::string Device::TEMP_A("temp_a");
const std::string Device::TEMP_B("temp_b");
const std::string Device::TEMP_C("temp_c");
const std::string Device::TEMP_D("temp_d");


void Device::describeComponent(ComponentInfo &info) {
    IODevice::describeComponent(info);
    
    info.setSignature("iodevice/blackrock_led_driver");
    
    info.addParameter(TEMP_A, false);
    info.addParameter(TEMP_B, false);
    info.addParameter(TEMP_C, false);
    info.addParameter(TEMP_D, false);
}


Device::Device(const ParameterValueMap &parameters) :
    IODevice(parameters),
    handle(nullptr),
    running(false)
{
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
    
    intensity.fill(0);
}


Device::~Device() {
    lock_guard lock(mutex);
    
    if (readTempsTask) {
        readTempsTask->cancel();
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
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Cannot open LED driver (status: %d)", status);
        return false;
    }
    
    if (FT_OK != (status = FT_SetBaudRate(handle, FT_BAUD_9600))) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Cannot set LED driver baud rate (status: %d)", status);
        return false;
    }
    
    boost::weak_ptr<Device> weakThis(component_shared_from_this<Device>());
    readTempsTask = Scheduler::instance()->scheduleUS(FILELINE,
                                                      0,
                                                      1000000,  // Check temps once per second
                                                      M_REPEAT_INDEFINITELY,
                                                      [weakThis]() {
                                                          if (auto sharedThis = weakThis.lock()) {
                                                              sharedThis->readTemps();
                                                          }
                                                          return nullptr;
                                                      },
                                                      M_DEFAULT_IODEVICE_PRIORITY,
                                                      M_DEFAULT_IODEVICE_WARN_SLOP_US,
                                                      M_DEFAULT_IODEVICE_FAIL_SLOP_US,
                                                      M_MISSED_EXECUTION_DROP);
    
    return true;
}


bool Device::startDeviceIO() {
    lock_guard lock(mutex);
    running = true;
    return true;
}


bool Device::stopDeviceIO() {
    lock_guard lock(mutex);
    running = false;
    return true;
}


void Device::setIntensity(const std::set<int> &channels, std::uint16_t value) {
    lock_guard lock(mutex);
    
    for (int channelNum : channels) {
        if ((channelNum < 1) || (channelNum > intensity.size())) {
            merror(M_IODEVICE_MESSAGE_DOMAIN, "Invalid channel number: %d", channelNum);
        } else {
            intensity[channelNum - 1] = value;
        }
    }
}


void Device::readTemps() {
    lock_guard lock(mutex);
    
    if (!readTempsTask) {
        // We've already been canceled, so don't try to read more data
        return;
    }
    
    FT_STATUS status;
    DWORD bytesAvailable;
    ThermistorValuesMessage msg;
    
    while (true) {
        if (FT_OK != (status = FT_GetQueueStatus(handle, &bytesAvailable))) {
            merror(M_IODEVICE_MESSAGE_DOMAIN, "Cannot determine size of LED driver read queue (status: %d)", status);
            break;
        }
        
        if (bytesAvailable < msg.size() ||
            !read(msg) ||
            !handleThermistorValuesMessage(msg))
        {
            break;
        }
    }
}


bool Device::handleThermistorValuesMessage(const ThermistorValuesMessage &msg) {
    if (!(msg.isCommand(thermistorValuesCommand))) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Unexpected message from LED driver");
        return false;
    }
    
    if (!(msg.testChecksum())) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Invalid checksum on message from LED driver");
        return false;
    }
    
    if (running) {
        announceTemp(tempA, msg.getBody().tempA.get());
        announceTemp(tempB, msg.getBody().tempB.get());
        announceTemp(tempC, msg.getBody().tempC.get());
        announceTemp(tempD, msg.getBody().tempD.get());
    }
    
    return true;
}


inline void Device::announceTemp(VariablePtr &var, std::uint16_t value) {
    if (var) {
        var->setValue(double(value) / 1000.0);
    }
    //mprintf(M_IODEVICE_MESSAGE_DOMAIN, "Read temp: %d", value);
}


bool Device::requestIntensityChange(std::uint8_t channel, std::uint16_t intensity) {
    SetIntensityMessage request;
    
    request.setCommand(setIntensityCommand);
    request.getBody().channel = channel;
    request.getBody().intensity.set(intensity);
    
    if (!write(request)) {
        return false;
    }
    
    union {
        SetIntensityMessage setIntensity;
        ThermistorValuesMessage thermistorValues;
    } response;
    
    while (true) {
        if (!read(response.setIntensity)) {
            return false;
        }
        
        if (!(response.setIntensity.isCommand(setIntensityCommand))) {
            
            //
            // Try to handle thermistor values
            //
            BOOST_STATIC_ASSERT(sizeof(response.setIntensity) < sizeof(response.thermistorValues));
            if (!(read(response.thermistorValues, response.setIntensity.size()) &&
                  handleThermistorValuesMessage(response.thermistorValues)))
            {
                return false;
            }
            
        } else {
            
            if (!(response.setIntensity.testChecksum())) {
                merror(M_IODEVICE_MESSAGE_DOMAIN, "LED driver response contained invalid checksum");
                return false;
            }
            
            if (response.setIntensity.getBody().channel != channel) {
                merror(M_IODEVICE_MESSAGE_DOMAIN, "LED driver responded with incorrect channel");
                return false;
            }
            
            if (response.setIntensity.getBody().intensity.get() != intensity) {
                merror(M_IODEVICE_MESSAGE_DOMAIN, "LED driver responded with incorrect intensity");
                return false;
            }
            
            break;
            
        }
    }
    
    return true;
}


template<typename Body>
bool Device::read(Message<Body> &msg, std::size_t bytesAlreadyRead) {
    const std::size_t bytesToRead = msg.size() - bytesAlreadyRead;
    FT_STATUS status;
    DWORD bytesRead;
    
    if (FT_OK != (status = FT_Read(handle, msg.data() + bytesAlreadyRead, bytesToRead, &bytesRead))) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Read from LED driver failed (status: %d)", status);
        return false;
    }
    
    if (bytesRead != bytesToRead) {
        merror(M_IODEVICE_MESSAGE_DOMAIN,
               "Incomplete read from LED driver (requested %lu bytes, read %d)",
               bytesToRead,
               bytesRead);
        return false;
    }
    
    //mprintf(M_IODEVICE_MESSAGE_DOMAIN, "Read message:\t%s", msg.hex().c_str());
    
    return true;
}


template<typename Body>
bool Device::write(Message<Body> &msg) {
    msg.setChecksum();
    
    FT_STATUS status;
    DWORD bytesWritten;
    
    if (FT_OK != (status = FT_Write(handle, msg.data(), msg.size(), &bytesWritten))) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Write to LED driver failed (status: %d)", status);
        return false;
    }
    
    if (bytesWritten != msg.size()) {
        merror(M_IODEVICE_MESSAGE_DOMAIN,
               "Incomplete write to LED driver (attempted %lu bytes, wrote %d)",
               msg.size(),
               bytesWritten);
        return false;
    }
    
    //mprintf(M_IODEVICE_MESSAGE_DOMAIN, "Wrote message:\t%s", msg.hex().c_str());
    
    return true;
}


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER


























