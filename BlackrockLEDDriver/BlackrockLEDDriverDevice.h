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
    
    void setIntensity(const std::set<int> &channels, std::uint16_t value);
    void setIntensity(int channelNum, std::uint16_t value);
    
private:
    static constexpr std::size_t numChannels = 64;
    
    FT_HANDLE handle;
    std::array<std::uint16_t, numChannels> intensity;
    
};


END_NAMESPACE_MW


#endif /* !defined(__BlackrockLEDDriver__BlackrockLEDDriverDevice__) */


























