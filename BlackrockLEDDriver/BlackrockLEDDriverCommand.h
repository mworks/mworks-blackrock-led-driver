//
//  BlackrockLEDDriverCommand.h
//  BlackrockLEDDriver
//
//  Created by Christopher Stawarz on 9/29/14.
//  Copyright (c) 2014 The MWorks Project. All rights reserved.
//

#ifndef BlackrockLEDDriver_BlackrockLEDDriverCommand_h
#define BlackrockLEDDriver_BlackrockLEDDriverCommand_h

#define BEGIN_NAMESPACE_MW_BLACKROCK_LEDDRIVER \
    BEGIN_NAMESPACE_MW BEGIN_NAMESPACE(blackrock) BEGIN_NAMESPACE(led_driver)

#define END_NAMESPACE_MW_BLACKROCK_LEDDRIVER \
    END_NAMESPACE(led_driver) END_NAMESPACE(blackrock) END_NAMESPACE_MW


BEGIN_NAMESPACE_MW_BLACKROCK_LEDDRIVER


using Command = std::array<std::uint8_t, 3>;
constexpr Command setIntensityCommand     = { 0x05, 0x05, 0x00 };
constexpr Command thermistorValuesCommand = { 0x05, 0x05, 0x80 };


struct TwoByteValue {
    
    std::uint16_t get() const {
        std::uint16_t value;
        getHighByte(value) = high;
        getLowByte(value) = low;
        return value;
    }
    
    void set(std::uint16_t value) {
        high = getHighByte(value);
        low = getLowByte(value);
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
    
    bool isCommand(const Command &cmd) const { return (command == cmd); }
    void setCommand(const Command &cmd) { command = cmd; }
    
    const Body& getBody() const { return body; }
    Body& getBody() { return const_cast<Body &>(static_cast<const Message &>(*this).getBody()); }
    
    bool testChecksum() const { return (checksum == computeChecksum()); }
    void setChecksum() { checksum = computeChecksum(); }
    
    std::size_t size() const { return sizeof(Message); }
    const std::uint8_t* data() const { return reinterpret_cast<const std::uint8_t *>(this); };
    std::uint8_t* data() { return const_cast<std::uint8_t *>(static_cast<const Message &>(*this).data()); };
    
    std::string hex() const {
        std::ostringstream os;
        for (auto iter = data(); iter != data() + size(); iter++) {
            os << std::hex << std::setfill('0') << std::setw(2) << int(*iter) << ' ';
        }
        return os.str();
    }
    
private:
    std::uint8_t computeChecksum() const {
        return std::accumulate(data(), data() + (size() - 1), std::uint8_t(0));
    }
    
    Command command;
    Body body;
    std::uint8_t checksum;
    
};


struct SetIntensityBody {
    std::uint8_t channel;
    TwoByteValue intensity;
};
using SetIntensityMessage = Message<SetIntensityBody>;


struct ThermistorValuesBody {
    TwoByteValue tempA;
    TwoByteValue tempB;
    TwoByteValue tempC;
    TwoByteValue tempD;
};
using ThermistorValuesMessage = Message<ThermistorValuesBody>;


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER


#endif /* !defined(BlackrockLEDDriver_BlackrockLEDDriverCommand_h) */


























