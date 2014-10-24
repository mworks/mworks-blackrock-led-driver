//
//  BlackrockLEDDriverSetIntensityAction.cpp
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 10/24/14.
//  Copyright (c) 2014 The MWorks Project. All rights reserved.
//

#include "BlackrockLEDDriverSetIntensityAction.h"

#include "BlackrockLEDDriverRunAction.h"


BEGIN_NAMESPACE_MW_BLACKROCK_LEDDRIVER


const std::string SetIntensityAction::CHANNELS("channels");
const std::string SetIntensityAction::VALUE("value");


void SetIntensityAction::describeComponent(ComponentInfo &info) {
    Action::describeComponent(info);
    
    info.setSignature("action/blackrock_led_driver_set_intensity");
    
    info.addParameter(CHANNELS);
    info.addParameter(VALUE);
}


SetIntensityAction::SetIntensityAction(const ParameterValueMap &parameters) :
    Action(parameters),
    channelList(ParsedExpressionVariable::parseExpressionList(parameters[CHANNELS].str())),
    value(parameters[VALUE])
{ }


bool SetIntensityAction::execute() {
    if (auto sharedDevice = weakDevice.lock()) {
        std::vector<Datum> channelNums;
        ParsedExpressionVariable::evaluateParseTreeList(channelList, channelNums);
        
        std::set<int> channels;
        for (auto &channelNum : channelNums) {
            channels.insert(channelNum.getInteger());
        }
        
        sharedDevice->setIntensity(channels, value->getValue().getFloat());
    }
    
    return true;
}


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER

























