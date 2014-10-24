//
//  BlackrockLEDDriverSetIntensityAction.h
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 10/24/14.
//  Copyright (c) 2014 The MWorks Project. All rights reserved.
//

#ifndef __BlackrockLEDDriver__BlackrockLEDDriverSetIntensityAction__
#define __BlackrockLEDDriver__BlackrockLEDDriverSetIntensityAction__

#include "BlackrockLEDDriverAction.h"


BEGIN_NAMESPACE_MW_BLACKROCK_LEDDRIVER


class SetIntensityAction : public Action {
    
public:
    static const std::string CHANNELS;
    static const std::string VALUE;
    
    static void describeComponent(ComponentInfo &info);
    
    explicit SetIntensityAction(const ParameterValueMap &parameters);
    
    bool execute() override;
    
private:
    const stx::ParseTreeList channelList;
    const VariablePtr value;
    
};


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER


#endif /* !defined(__BlackrockLEDDriver__BlackrockLEDDriverSetIntensityAction__) */
