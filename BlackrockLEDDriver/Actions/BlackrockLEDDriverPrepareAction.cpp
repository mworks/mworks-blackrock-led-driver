//
//  BlackrockLEDDriverPrepareAction.cpp
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 6/14/16.
//  Copyright © 2016 The MWorks Project. All rights reserved.
//

#include "BlackrockLEDDriverPrepareAction.hpp"


BEGIN_NAMESPACE_MW_BLACKROCK_LEDDRIVER


const std::string PrepareAction::DURATION("duration");


void PrepareAction::describeComponent(ComponentInfo &info) {
    Action::describeComponent(info);
    
    info.setSignature("action/blackrock_led_driver_prepare");
    
    info.addParameter(DURATION);
}


PrepareAction::PrepareAction(const ParameterValueMap &parameters) :
    Action(parameters),
    duration(parameters[DURATION])
{ }


bool PrepareAction::execute() {
    if (auto sharedDevice = weakDevice.lock()) {
        sharedDevice->prepare(duration->getValue().getInteger());
    }
    return true;
}


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER

























