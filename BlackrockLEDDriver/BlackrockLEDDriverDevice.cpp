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
    IODevice(parameters)
{
}


BlackrockLEDDriverDevice::~BlackrockLEDDriverDevice() {
}


bool BlackrockLEDDriverDevice::initialize() {
    return true;
}


bool BlackrockLEDDriverDevice::startDeviceIO() {
    return true;
}


bool BlackrockLEDDriverDevice::stopDeviceIO() {
    return true;
}


END_NAMESPACE_MW


























