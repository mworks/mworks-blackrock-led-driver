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


template<std::uint8_t c0, std::uint8_t c1, std::uint8_t c2, typename Body>
struct Message {
    
    bool testCommand() const { return (command == Command{ c0, c1, c2 }); }
    
    const Body& getBody() const { return body; }
    Body& getBody() { return const_cast<Body &>(static_cast<const Message &>(*this).getBody()); }
    
    bool testChecksum() const { return (checksum == computeChecksum()); }
    
    bool read(FT_HANDLE handle, std::size_t bytesAlreadyRead = 0);
    bool write(FT_HANDLE handle);
    
    static constexpr std::size_t size() { return sizeof(Message); }
    
    const std::uint8_t* data() const { return reinterpret_cast<const std::uint8_t *>(this); };
    std::uint8_t* data() { return const_cast<std::uint8_t *>(static_cast<const Message &>(*this).data()); };
    
    using const_iterator = const std::uint8_t *;
    const_iterator begin() const { return data(); }
    const_iterator end() const { return begin() + size(); }
    
    std::string hex() const {
        std::ostringstream os;
        for (std::uint8_t byte : *this) {
            os << std::hex << std::setfill('0') << std::setw(2) << int(byte) << ' ';
        }
        return os.str();
    }
    
private:
    std::uint8_t computeChecksum() const {
        return std::accumulate(begin(), end() - 1, std::uint8_t(0));
    }
    
    using Command = std::array<std::uint8_t, 3>;
    
    Command command;
    Body body;
    std::uint8_t checksum;
    
};


template<std::uint8_t c0, std::uint8_t c1, std::uint8_t c2, typename Body>
bool Message<c0, c1, c2, Body>::read(FT_HANDLE handle, std::size_t bytesAlreadyRead) {
    FT_STATUS status;
    const std::size_t bytesToRead = size() - bytesAlreadyRead;
    DWORD bytesRead;
    
    if (FT_OK != (status = FT_Read(handle, data() + bytesAlreadyRead, bytesToRead, &bytesRead))) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Read from LED driver failed (status: %d)", status);
        return false;
    }
    
    if (bytesRead != bytesToRead) {
        merror(M_IODEVICE_MESSAGE_DOMAIN,
               "Incomplete read from LED driver (requested %lu bytes, read %d)",
               bytesToRead,
               bytesRead);
        return false;
    }
    
    //mprintf(M_IODEVICE_MESSAGE_DOMAIN, "Read message:\t%s", hex().c_str());
    
    return true;
}


template<std::uint8_t c0, std::uint8_t c1, std::uint8_t c2, typename Body>
bool Message<c0, c1, c2, Body>::write(FT_HANDLE handle) {
    command = { c0, c1, c2 };
    checksum = computeChecksum();
    
    FT_STATUS status;
    DWORD bytesWritten;
    
    if (FT_OK != (status = FT_Write(handle, data(), size(), &bytesWritten))) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Write to LED driver failed (status: %d)", status);
        return false;
    }
    
    if (bytesWritten != size()) {
        merror(M_IODEVICE_MESSAGE_DOMAIN,
               "Incomplete write to LED driver (attempted %lu bytes, wrote %d)",
               size(),
               bytesWritten);
        return false;
    }
    
    //mprintf(M_IODEVICE_MESSAGE_DOMAIN, "Wrote message:\t%s", hex().c_str());
    
    return true;
}


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


struct SetIntensityMessageBody {
    std::uint8_t channel;
    TwoByteValue intensity;
};
using SetIntensityMessage = Message<0x05, 0x05, 0x00, SetIntensityMessageBody>;


struct ThermistorValuesMessageBody {
    TwoByteValue tempA;
    TwoByteValue tempB;
    TwoByteValue tempC;
    TwoByteValue tempD;
};
using ThermistorValuesMessage = Message<0x05, 0x05, 0x80, ThermistorValuesMessageBody>;


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER


#endif /* !defined(BlackrockLEDDriver_BlackrockLEDDriverCommand_h) */


























