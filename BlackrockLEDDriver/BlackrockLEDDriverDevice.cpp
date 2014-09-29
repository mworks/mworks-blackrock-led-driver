//
//  BlackrockLEDDriverDevice.cpp
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 9/24/14.
//  Copyright (c) 2014 The MWorks Project. All rights reserved.
//

#include "BlackrockLEDDriverDevice.h"

#include <numeric>


BEGIN_NAMESPACE_MW


const std::string BlackrockLEDDriverDevice::TEMP_A("temp_a");
const std::string BlackrockLEDDriverDevice::TEMP_B("temp_b");
const std::string BlackrockLEDDriverDevice::TEMP_C("temp_c");
const std::string BlackrockLEDDriverDevice::TEMP_D("temp_d");


void BlackrockLEDDriverDevice::describeComponent(ComponentInfo &info) {
    IODevice::describeComponent(info);
    
    info.setSignature("iodevice/blackrock_led_driver");
    
    info.addParameter(TEMP_A, false);
    info.addParameter(TEMP_B, false);
    info.addParameter(TEMP_C, false);
    info.addParameter(TEMP_D, false);
}


BlackrockLEDDriverDevice::BlackrockLEDDriverDevice(const ParameterValueMap &parameters) :
    IODevice(parameters),
    handle(nullptr),
    running(false)
{
    if (!(parameters[TEMP_A].empty())) {
        tempA = VariablePtr(parameters[TEMP_A]);
    }
    if (!(parameters[TEMP_B].empty())) {
        tempB = VariablePtr(parameters[TEMP_B]);
    }
    if (!(parameters[TEMP_C].empty())) {
        tempC = VariablePtr(parameters[TEMP_C]);
    }
    if (!(parameters[TEMP_D].empty())) {
        tempD = VariablePtr(parameters[TEMP_D]);
    }
    
    intensity.fill(0);
}


BlackrockLEDDriverDevice::~BlackrockLEDDriverDevice() {
    lock_guard lock(mutex);
    
    if (readTempsTask) {
        readTempsTask->cancel();
    }
    
    if (handle) {
        FT_STATUS status = FT_Close(handle);
        if (FT_OK != status) {
            merror(M_IODEVICE_MESSAGE_DOMAIN, "Cannot close LED driver (status: %d)", status);
        }
    }
}


bool BlackrockLEDDriverDevice::initialize() {
    lock_guard lock(mutex);
    
    FT_STATUS status;
    
    if (FT_OK != (status = FT_OpenEx(const_cast<char *>("Blinky 1.0"), FT_OPEN_BY_DESCRIPTION, &handle))) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Cannot open LED driver (status: %d)", status);
        return false;
    }
    
    if (FT_OK != (status = FT_SetBaudRate(handle, FT_BAUD_9600))) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Cannot set LED driver baud rate (status: %d)", status);
        return false;
    }
    
    boost::weak_ptr<BlackrockLEDDriverDevice> weakThis(component_shared_from_this<BlackrockLEDDriverDevice>());
    readTempsTask = Scheduler::instance()->scheduleUS(FILELINE,
                                                      0,
                                                      1000000,  // Check temps once per second
                                                      M_REPEAT_INDEFINITELY,
                                                      [weakThis]() {
                                                          if (auto sharedThis = weakThis.lock()) {
                                                              sharedThis->readTemps();
                                                          }
                                                          return nullptr;
                                                      },
                                                      M_DEFAULT_IODEVICE_PRIORITY,
                                                      M_DEFAULT_IODEVICE_WARN_SLOP_US,
                                                      M_DEFAULT_IODEVICE_FAIL_SLOP_US,
                                                      M_MISSED_EXECUTION_DROP);
    
    return true;
}


bool BlackrockLEDDriverDevice::startDeviceIO() {
    lock_guard lock(mutex);
    running = true;
    return true;
}


bool BlackrockLEDDriverDevice::stopDeviceIO() {
    lock_guard lock(mutex);
    running = false;
    return true;
}


void BlackrockLEDDriverDevice::setIntensity(const std::set<int> &channels, std::uint16_t value) {
    for (int channelNum : channels) {
        setIntensity(channelNum, value);
    }
}


void BlackrockLEDDriverDevice::setIntensity(int channelNum, std::uint16_t value) {
    if ((channelNum < 1) || (channelNum > intensity.size())) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Invalid channel number: %d", channelNum);
    } else {
        intensity[channelNum - 1] = value;
    }
}


namespace {
    struct TwoByteValue {
        std::uint16_t get() const {
            std::uint16_t value;
            getLowByte(value) = low;
            getHighByte(value) = high;
            return value;
        }
        
        void set(std::uint16_t value) {
            low = getLowByte(value);
            high = getHighByte(value);
        }
        
    private:
        static std::uint8_t& getByte(std::uint16_t &value, std::size_t index) {
            return reinterpret_cast<std::uint8_t *>(&value)[index];
        }
        static std::uint8_t& getLowByte(std::uint16_t &value) { return getByte(value, lowByteIndex); }
        static std::uint8_t& getHighByte(std::uint16_t &value) { return getByte(value, highByteIndex); }
        
        BOOST_STATIC_ASSERT(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__);
        static constexpr std::size_t lowByteIndex = 0;
        static constexpr std::size_t highByteIndex = 1;
        
        std::uint8_t high;
        std::uint8_t low;
    };
    
    
    template<typename Body>
    struct Message {
        using command_type = std::array<std::uint8_t, 3>;
        
        bool isCommand(const command_type &cmd) const { return (command == cmd); }
        void setCommand(const command_type &cmd) { command = cmd; }
        
        const Body& getBody() const { return body; }
        Body& getBody() { return const_cast<Body &>(static_cast<const Message &>(*this).getBody()); }
        
        bool testChecksum() const { return (checksum == computeChecksum()); }
        void setChecksum() { checksum = computeChecksum(); }
        
        std::size_t size() const { return sizeof(Message); }
        const std::uint8_t* data() const { return reinterpret_cast<const std::uint8_t *>(this); };
        std::uint8_t* data() { return const_cast<std::uint8_t *>(static_cast<const Message &>(*this).data()); };
        
        void log() const {
            std::ostringstream os;
            for (auto iter = data(); iter != data() + size(); iter++) {
                os << std::hex << std::setfill('0') << std::setw(2) << int(*iter) << ' ';
            }
            mprintf(M_IODEVICE_MESSAGE_DOMAIN, "Received message: %s", os.str().c_str());
        }
        
    private:
        std::uint8_t computeChecksum() const {
            return std::accumulate(data(), data() + (size() - 1), std::uint8_t(0));
        }
        
        command_type command;
        Body body;
        std::uint8_t checksum;
    };
    
    
    struct SetIntensityMessageBody {
        std::uint8_t channel;
        TwoByteValue intensity;
    };
    using SetIntensityMessage = Message<SetIntensityMessageBody>;
    
    
    struct ThermistorValuesMessageBody {
        TwoByteValue tempA;
        TwoByteValue tempB;
        TwoByteValue tempC;
        TwoByteValue tempD;
    };
    using ThermistorValuesMessage = Message<ThermistorValuesMessageBody>;
}


void BlackrockLEDDriverDevice::readTemps() {
    lock_guard lock(mutex);
    
    if (!readTempsTask) {
        // We've already been canceled, so don't try to read more data
        return;
    }
    
    FT_STATUS status;
    DWORD bytesAvailable, bytesRead;
    ThermistorValuesMessage msg;
    
    while (true) {
        if (FT_OK != (status = FT_GetQueueStatus(handle, &bytesAvailable))) {
            merror(M_IODEVICE_MESSAGE_DOMAIN, "Cannot determine size of LED driver read queue (status: %d)", status);
            break;
        }
        
        if (bytesAvailable < msg.size()) {
            break;
        }
        
        if (FT_OK != (status = FT_Read(handle, msg.data(), msg.size(), &bytesRead))) {
            merror(M_IODEVICE_MESSAGE_DOMAIN, "Read from LED driver failed (status: %d)", status);
            break;
        }
        
        if (bytesRead != msg.size()) {
            merror(M_IODEVICE_MESSAGE_DOMAIN,
                   "Incomplete read from LED driver failed (requested %lu bytes, got %d)",
                   msg.size(),
                   bytesRead);
            break;
        }
        
        if (!(msg.testChecksum())) {
            merror(M_IODEVICE_MESSAGE_DOMAIN, "Invalid checksum on message from LED driver");
            break;
        }
        
        if (!(msg.isCommand({0x05, 0x05, 0x80}))) {
            merror(M_IODEVICE_MESSAGE_DOMAIN, "Unexpected message from LED driver");
            break;
        }
        
        announceTemp(tempA, msg.getBody().tempA.get());
        announceTemp(tempB, msg.getBody().tempB.get());
        announceTemp(tempC, msg.getBody().tempC.get());
        announceTemp(tempD, msg.getBody().tempD.get());
    }
}


void BlackrockLEDDriverDevice::announceTemp(VariablePtr &var, std::uint16_t value) {
    if (var) {
        var->setValue(double(value) / 1000.0);
    }
}


END_NAMESPACE_MW


























