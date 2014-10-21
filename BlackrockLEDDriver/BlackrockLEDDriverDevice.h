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


BEGIN_NAMESPACE_MW_BLACKROCK_LEDDRIVER


class Device : public IODevice, boost::noncopyable {
    
public:
    static const std::string TEMP_A;
    static const std::string TEMP_B;
    static const std::string TEMP_C;
    static const std::string TEMP_D;
    
    static void describeComponent(ComponentInfo &info);
    
    explicit Device(const ParameterValueMap &parameters);
    ~Device();
    
    bool initialize() override;
    bool startDeviceIO() override;
    bool stopDeviceIO() override;
    
    void setIntensity(const std::set<int> &channels, WORD value);
    
private:
    void readTemps();
    bool handleThermistorValuesMessage(const ThermistorValuesMessage &msg);
    void announceTemp(VariablePtr &var, WORD value);
    
    bool requestIntensityChange(BYTE channel, WORD value);
    
    template<typename Request, typename Response>
    bool perform(Request &request, Response &response);
    
    template<typename Message>
    bool perform(Message &message) { return perform(message, message); }
    
    VariablePtr tempA;
    VariablePtr tempB;
    VariablePtr tempC;
    VariablePtr tempD;
    
    FT_HANDLE handle;
    std::array<WORD, numChannels> intensity;
    
    boost::shared_ptr<ScheduleTask> readTempsTask;
    
    std::mutex mutex;
    using lock_guard = std::lock_guard<std::mutex>;
    
    bool running;
    
};


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER


#endif /* !defined(__BlackrockLEDDriver__BlackrockLEDDriverDevice__) */


























