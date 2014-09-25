//
//  BlackrockLEDDriverDevice.h
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 9/24/14.
//  Copyright (c) 2014 The MWorks Project. All rights reserved.
//

#ifndef __BlackrockLEDDriver__BlackrockLEDDriverDevice__
#define __BlackrockLEDDriver__BlackrockLEDDriverDevice__


BEGIN_NAMESPACE_MW


class BlackrockLEDDriverDevice : public IODevice, boost::noncopyable {
    
public:
    static void describeComponent(ComponentInfo &info);
    
    explicit BlackrockLEDDriverDevice(const ParameterValueMap &parameters);
    ~BlackrockLEDDriverDevice();
    
    bool initialize() override;
    bool startDeviceIO() override;
    bool stopDeviceIO() override;
    
private:
    
};


END_NAMESPACE_MW


#endif /* !defined(__BlackrockLEDDriver__BlackrockLEDDriverDevice__) */


























