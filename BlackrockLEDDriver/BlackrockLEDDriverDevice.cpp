//
//  BlackrockLEDDriverDevice.cpp
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 9/24/14.
//  Copyright (c) 2014 The MWorks Project. All rights reserved.
//

#include "BlackrockLEDDriverDevice.h"


BEGIN_NAMESPACE_MW


void BlackrockLEDDriverDevice::describeComponent(ComponentInfo &info) {
    IODevice::describeComponent(info);
    info.setSignature("iodevice/blackrock_led_driver");
}


BlackrockLEDDriverDevice::BlackrockLEDDriverDevice(const ParameterValueMap &parameters) :
    IODevice(parameters),
    handle(nullptr)
{
    intensity.fill(0);
}


BlackrockLEDDriverDevice::~BlackrockLEDDriverDevice() {
    if (handle) {
        FT_STATUS status = FT_Close(handle);
        if (FT_OK != status) {
            merror(M_IODEVICE_MESSAGE_DOMAIN, "Cannot close LED driver (status: %d)", status);
        }
    }
}


bool BlackrockLEDDriverDevice::initialize() {
    FT_STATUS status = FT_OpenEx(const_cast<char *>("Blinky 1.0"), FT_OPEN_BY_DESCRIPTION, &handle);
    if (FT_OK != status) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Cannot open LED driver (status: %d)", status);
        return false;
    }
    
    return true;
}


bool BlackrockLEDDriverDevice::startDeviceIO() {
    return true;
}


bool BlackrockLEDDriverDevice::stopDeviceIO() {
    return true;
}


void BlackrockLEDDriverDevice::setIntensity(const std::set<int> &channels, std::uint16_t value) {
    for (int channelNum : channels) {
        setIntensity(channelNum, value);
    }
}


void BlackrockLEDDriverDevice::setIntensity(int channelNum, std::uint16_t value) {
    if ((channelNum < 1) || (channelNum > intensity.size())) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Invalid channel number: %d", channelNum);
    } else {
        intensity.at(channelNum - 1) = value;
    }
}


END_NAMESPACE_MW


























