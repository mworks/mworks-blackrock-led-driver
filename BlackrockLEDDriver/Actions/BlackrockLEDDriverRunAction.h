//
//  BlackrockLEDDriverRunAction.h
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 10/23/14.
//  Copyright (c) 2014 The MWorks Project. All rights reserved.
//

#ifndef __BlackrockLEDDriver__BlackrockLEDDriverRunAction__
#define __BlackrockLEDDriver__BlackrockLEDDriverRunAction__

#include "BlackrockLEDDriverAction.h"


BEGIN_NAMESPACE_MW_BLACKROCK_LEDDRIVER


class RunAction : public Action {
    
public:
    static const std::string DURATION;
    
    static void describeComponent(ComponentInfo &info);
    
    explicit RunAction(const ParameterValueMap &parameters);
    
    bool execute() override;
    
private:
    const VariablePtr duration;
    
};


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER


#endif /* !defined(__BlackrockLEDDriver__BlackrockLEDDriverRunAction__) */

























