//
//  BlackrockLEDDriverReadTempsAction.cpp
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 6/14/16.
//  Copyright © 2016 The MWorks Project. All rights reserved.
//

#include "BlackrockLEDDriverReadTempsAction.hpp"


BEGIN_NAMESPACE_MW_BLACKROCK_LEDDRIVER


void ReadTempsAction::describeComponent(ComponentInfo &info) {
    Action::describeComponent(info);
    info.setSignature("action/blackrock_led_driver_read_temps");
}


bool ReadTempsAction::execute() {
    if (auto sharedDevice = weakDevice.lock()) {
        sharedDevice->readTemps();
    }
    return true;
}


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER

























