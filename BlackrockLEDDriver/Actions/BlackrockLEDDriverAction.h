//
//  BlackrockLEDDriverAction.h
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 10/24/14.
//  Copyright (c) 2014 The MWorks Project. All rights reserved.
//

#ifndef __BlackrockLEDDriver__BlackrockLEDDriverAction__
#define __BlackrockLEDDriver__BlackrockLEDDriverAction__

#include "BlackrockLEDDriverDevice.h"


BEGIN_NAMESPACE_MW_BLACKROCK_LEDDRIVER


class Action : public mw::Action {
    
public:
    static const std::string DEVICE;
    
    static void describeComponent(ComponentInfo &info);
    
    explicit Action(const ParameterValueMap &parameters);
    
protected:
    const boost::weak_ptr<Device> weakDevice;
    
};


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER


#endif /* !defined(__BlackrockLEDDriver__BlackrockLEDDriverAction__) */
