//
//  BlackrockLEDDriverRunAction.cpp
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 10/23/14.
//  Copyright (c) 2014 The MWorks Project. All rights reserved.
//

#include "BlackrockLEDDriverRunAction.h"


BEGIN_NAMESPACE_MW_BLACKROCK_LEDDRIVER


const std::string RunAction::DURATION("duration");


void RunAction::describeComponent(ComponentInfo &info) {
    Action::describeComponent(info);
    
    info.setSignature("action/blackrock_led_driver_run");
    
    info.addParameter(DURATION);
}


RunAction::RunAction(const ParameterValueMap &parameters) :
    Action(parameters),
    duration(parameters[DURATION])
{ }


bool RunAction::execute() {
    if (auto sharedDevice = weakDevice.lock()) {
        sharedDevice->run(duration->getValue().getInteger());
    }
    return true;
}


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER

























