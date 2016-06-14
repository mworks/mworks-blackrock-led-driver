//
//  BlackrockLEDDriverReadTempsAction.hpp
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 6/14/16.
//  Copyright © 2016 The MWorks Project. All rights reserved.
//

#ifndef BlackrockLEDDriverReadTempsAction_hpp
#define BlackrockLEDDriverReadTempsAction_hpp

#include "BlackrockLEDDriverAction.h"


BEGIN_NAMESPACE_MW_BLACKROCK_LEDDRIVER


class ReadTempsAction : public Action {
    
public:
    static void describeComponent(ComponentInfo &info);
    
    using Action::Action;
    
    bool execute() override;
    
};


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER


#endif /* BlackrockLEDDriverReadTempsAction_hpp */

























