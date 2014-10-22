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
    filePlaying(false)
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
    
    intensity.fill(0);
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
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Cannot open LED driver (status: %d)", status);
        return false;
    }
    
    if (FT_OK != (status = FT_SetBaudRate(handle, FT_BAUD_9600))) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Cannot set LED driver baud rate (status: %d)", status);
        return false;
    }
    
    boost::weak_ptr<Device> weakThis(component_shared_from_this<Device>());
    checkStatusTask = Scheduler::instance()->scheduleUS(FILELINE,
                                                        0,
                                                        500000,  // Check status every 500ms
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
}


void Device::run(MWTime duration) {
    lock_guard lock(mutex);
    
    filePlaying = true;
    if (running && !running->getValue().getBool()) {
        running->setValue(true);
    }
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
    
    if (filePlaying) {
        IsFilePlayingRequest request;
        IsFilePlayingResponse response;
        
        if (!perform(request, response)) {
            return;
        }
        
        if (!response.getBody().filePlaying) {
            filePlaying = false;
            if (running && running->getValue().getBool()) {
                running->setValue(false);
            }
        }
    }
    
    FT_STATUS status;
    DWORD bytesAvailable;
    ThermistorValuesMessage msg;
    
    while (true) {
        if (FT_OK != (status = FT_GetQueueStatus(handle, &bytesAvailable))) {
            merror(M_IODEVICE_MESSAGE_DOMAIN, "Cannot determine size of LED driver read queue (status: %d)", status);
            break;
        }
        
        if (bytesAvailable < msg.size() || !handleThermistorValuesMessage(msg)) {
            break;
        }
    }
}


bool Device::handleThermistorValuesMessage(ThermistorValuesMessage &msg, std::size_t bytesAlreadyRead) {
    if (!msg.read(handle, bytesAlreadyRead)) {
        return false;
    }
    
    if (!msg.testCommand()) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Unexpected message from LED driver");
        return false;
    }
    
    if (!msg.testChecksum()) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Invalid checksum on message from LED driver");
        return false;
    }
    
    announceTemp(tempA, msg.getBody().tempA);
    announceTemp(tempB, msg.getBody().tempB);
    announceTemp(tempC, msg.getBody().tempC);
    announceTemp(tempD, msg.getBody().tempD);
    
    return true;
}


inline void Device::announceTemp(VariablePtr &var, WORD value) {
    if (var) {
        var->setValue(double(value) / 1000.0);
    }
}


template<typename Request, typename Response>
bool Device::perform(Request &request, Response &response) {
    if (!request.write(handle)) {
        return false;
    }
    
    union {
        Response expectedResponse;
        ThermistorValuesMessage thermistorValues;
    } responseBuffer;
    
    BOOST_STATIC_ASSERT(sizeof(responseBuffer.expectedResponse) < sizeof(responseBuffer.thermistorValues));
    
    while (true) {
        if (!responseBuffer.expectedResponse.read(handle)) {
            return false;
        }
        
        if (responseBuffer.expectedResponse.testCommand()) {
            
            if (!(responseBuffer.expectedResponse.testChecksum())) {
                merror(M_IODEVICE_MESSAGE_DOMAIN, "LED driver response contained invalid checksum");
                return false;
            }
            
            break;
            
        } else {
            
            //
            // Try to handle thermistor values
            //
            if (!handleThermistorValuesMessage(responseBuffer.thermistorValues,
                                               responseBuffer.expectedResponse.size()))
            {
                return false;
            }
            
        }
    }
    
    response = responseBuffer.expectedResponse;
    
    return true;
}


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER


























