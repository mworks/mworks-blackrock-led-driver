//
//  BlackrockLEDDriverPrepareAction.hpp
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 6/14/16.
//  Copyright © 2016 The MWorks Project. All rights reserved.
//

#ifndef BlackrockLEDDriverPrepareAction_hpp
#define BlackrockLEDDriverPrepareAction_hpp

#include "BlackrockLEDDriverAction.h"


BEGIN_NAMESPACE_MW_BLACKROCK_LEDDRIVER


class PrepareAction : public Action {
    
public:
    static const std::string DURATION;
    
    static void describeComponent(ComponentInfo &info);
    
    explicit PrepareAction(const ParameterValueMap &parameters);
    
    bool execute() override;
    
private:
    const VariablePtr duration;
    
};


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER


#endif /* BlackrockLEDDriverPrepareAction_hpp */

























