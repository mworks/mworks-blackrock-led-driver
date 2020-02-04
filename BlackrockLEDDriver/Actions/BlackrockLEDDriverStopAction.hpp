//
//  BlackrockLEDDriverStopAction.hpp
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 2/4/20.
//  Copyright © 2020 The MWorks Project. All rights reserved.
//

#ifndef BlackrockLEDDriverStopAction_hpp
#define BlackrockLEDDriverStopAction_hpp

#include "BlackrockLEDDriverAction.h"


BEGIN_NAMESPACE_MW_BLACKROCK_LEDDRIVER


class StopAction : public Action {
    
public:
    static void describeComponent(ComponentInfo &info);
    
    using Action::Action;
    
    bool execute() override;
    
};


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER


#endif /* BlackrockLEDDriverStopAction_hpp */
