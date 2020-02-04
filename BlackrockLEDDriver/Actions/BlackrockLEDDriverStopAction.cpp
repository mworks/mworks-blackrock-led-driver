//
//  BlackrockLEDDriverStopAction.cpp
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 2/4/20.
//  Copyright © 2020 The MWorks Project. All rights reserved.
//

#include "BlackrockLEDDriverStopAction.hpp"


BEGIN_NAMESPACE_MW_BLACKROCK_LEDDRIVER


void StopAction::describeComponent(ComponentInfo &info) {
    Action::describeComponent(info);
    info.setSignature("action/blackrock_led_driver_stop");
}


bool StopAction::execute() {
    if (auto sharedDevice = weakDevice.lock()) {
        sharedDevice->stop();
    }
    return true;
}


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER
