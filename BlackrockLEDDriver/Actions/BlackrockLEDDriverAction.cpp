//
//  BlackrockLEDDriverAction.cpp
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 10/24/14.
//  Copyright (c) 2014 The MWorks Project. All rights reserved.
//

#include "BlackrockLEDDriverAction.h"


BEGIN_NAMESPACE_MW_BLACKROCK_LEDDRIVER


const std::string Action::DEVICE("device");


void Action::describeComponent(ComponentInfo &info) {
    mw::Action::describeComponent(info);
    info.addParameter(DEVICE);
}


Action::Action(const ParameterValueMap &parameters) :
    mw::Action(parameters),
    weakDevice(parameters[DEVICE].getRegistry()->getObject<Device>(parameters[DEVICE].str()))
{
    if (weakDevice.expired()) {
        throw SimpleException(M_IODEVICE_MESSAGE_DOMAIN,
                              "Device is not a Blackrock LED driver",
                              parameters[DEVICE].str());
    }
}


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER
