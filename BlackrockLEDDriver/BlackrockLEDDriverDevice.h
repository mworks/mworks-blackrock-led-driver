//
//  BlackrockLEDDriverDevice.h
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 9/24/14.
//  Copyright (c) 2014 The MWorks Project. All rights reserved.
//

#ifndef __BlackrockLEDDriver__BlackrockLEDDriverDevice__
#define __BlackrockLEDDriver__BlackrockLEDDriverDevice__

#include "BlackrockLEDDriverCommand.h"


BEGIN_NAMESPACE_MW


class BlackrockLEDDriverDevice : public IODevice, boost::noncopyable {
    
public:
    static const std::string TEMP_A;
    static const std::string TEMP_B;
    static const std::string TEMP_C;
    static const std::string TEMP_D;
    
    static void describeComponent(ComponentInfo &info);
    
    explicit BlackrockLEDDriverDevice(const ParameterValueMap &parameters);
    ~BlackrockLEDDriverDevice();
    
    bool initialize() override;
    bool startDeviceIO() override;
    bool stopDeviceIO() override;
    
    void setIntensity(const std::set<int> &channels, std::uint16_t value);
    
private:
    void readTemps();
    bool handleThermistorValuesMessage(const ThermistorValuesMessage &msg);
    void announceTemp(VariablePtr &var, std::uint16_t value);
    
    bool requestIntensityChange(std::uint8_t channel, std::uint16_t value);
    
    template<typename Body> bool read(Message<Body> &msg, std::size_t bytesAlreadyRead = 0);
    template<typename Body> bool write(Message<Body> &msg);
    
    static constexpr std::size_t numChannels = 64;
    
    VariablePtr tempA;
    VariablePtr tempB;
    VariablePtr tempC;
    VariablePtr tempD;
    
    FT_HANDLE handle;
    std::array<std::uint16_t, numChannels> intensity;
    
    boost::shared_ptr<ScheduleTask> readTempsTask;
    
    std::mutex mutex;
    using lock_guard = std::lock_guard<std::mutex>;
    
    bool running;
    
};


END_NAMESPACE_MW


#endif /* !defined(__BlackrockLEDDriver__BlackrockLEDDriverDevice__) */


























