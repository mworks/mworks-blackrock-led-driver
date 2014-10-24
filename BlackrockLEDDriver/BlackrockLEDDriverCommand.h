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

//#define MW_BLACKROCK_LEDDRIVER_DEBUG


BEGIN_NAMESPACE_MW_BLACKROCK_LEDDRIVER


// Confirm our assumptions about type aliases
BOOST_STATIC_ASSERT(std::is_same<BYTE, std::uint8_t>::value);
BOOST_STATIC_ASSERT(std::is_same<WORD, std::uint16_t>::value);


constexpr std::size_t numChannels = 64;
constexpr std::size_t numSamples = 50;

constexpr MWTime periodIncrement = 100;
constexpr MWTime minPeriod = periodIncrement * 1;  // 100 us
constexpr MWTime maxPeriod = periodIncrement * (1 + MWTime(std::numeric_limits<WORD>::max()));  // 6.5536 s

constexpr MWTime minDuration = minPeriod;  // 100 us
constexpr MWTime maxDuration = maxPeriod * numSamples;  // 327.68 s


struct EmptyMessageBody { };


template<BYTE c0, BYTE c1, BYTE c2, typename Body = EmptyMessageBody>
struct Message {
    
    bool testCommand() const { return (command == Command{ c0, c1, c2 }); }
    
    const Body& getBody() const { return bodyAndChecksum; }
    Body& getBody() { return const_cast<Body &>(static_cast<const Message &>(*this).getBody()); }
    
    bool testChecksum() const { return (bodyAndChecksum.checksum == computeChecksum()); }
    
    bool read(FT_HANDLE handle, std::size_t bytesAlreadyRead = 0);
    bool write(FT_HANDLE handle);
    
    static constexpr std::size_t size() { return sizeof(Message); }
    
    const BYTE* data() const { return reinterpret_cast<const BYTE *>(this); };
    BYTE* data() { return const_cast<BYTE *>(static_cast<const Message &>(*this).data()); };
    
    using const_iterator = const BYTE *;
    const_iterator begin() const { return data(); }
    const_iterator end() const { return begin() + size(); }
    
    std::string hex() const {
        std::ostringstream os;
        for (BYTE byte : *this) {
            os << std::hex << std::setfill('0') << std::setw(2) << int(byte) << ' ';
        }
        return os.str();
    }
    
private:
    BYTE computeChecksum() const {
        return std::accumulate(begin(), end() - 1, BYTE(0));
    }
    
    using Command = std::array<BYTE, 3>;
    
    struct BodyAndChecksum : public Body {
        BYTE checksum;
    };
    // If Body is empty, confirm that the empty base optimization is applied
    BOOST_STATIC_ASSERT(!std::is_empty<Body>::value || sizeof(BodyAndChecksum) == sizeof(BYTE));
    
    Command command;
    BodyAndChecksum bodyAndChecksum;
    
};


template<BYTE c0, BYTE c1, BYTE c2, typename Body>
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
    
#ifdef MW_BLACKROCK_LEDDRIVER_DEBUG
    mprintf(M_IODEVICE_MESSAGE_DOMAIN, "Read message:\t%s", hex().c_str());
#endif
    
    return true;
}


template<BYTE c0, BYTE c1, BYTE c2, typename Body>
bool Message<c0, c1, c2, Body>::write(FT_HANDLE handle) {
    command = { c0, c1, c2 };
    bodyAndChecksum.checksum = computeChecksum();
    
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
    
#ifdef MW_BLACKROCK_LEDDRIVER_DEBUG
    mprintf(M_IODEVICE_MESSAGE_DOMAIN, "Wrote message:\t%s", hex().c_str());
#endif
    
    return true;
}


struct WordValue {
    static WordValue zero() {
        return (WordValue() = 0);
    }
    
    operator WORD() const {
        return CFSwapInt16BigToHost(reinterpret_cast<const WORD &>(*this));
    }
    
    WordValue& operator=(WORD value) {
        reinterpret_cast<WORD &>(*this) = CFSwapInt16HostToBig(value);
        return (*this);
    }
    
private:
    BYTE highByte;
    BYTE lowByte;
};


struct LoadFileRequestBody {
    std::array<std::array<WordValue, numChannels>, numSamples> samples;
};
using LoadFileRequest = Message<0x05, 0x05, 0x04, LoadFileRequestBody>;


struct LoadFileResponseBody {
    BYTE fileLoaded;
};
using LoadFileResponse = Message<0x05, 0x05, 0x04, LoadFileResponseBody>;


struct SetFileTimePeriodMessageBody {
    WordValue period;
};
using SetFileTimePeriodMessage = Message<0x05, 0x05, 0x06, SetFileTimePeriodMessageBody>;


using StartFilePlayingRequest = Message<0x05, 0x05, 0x07>;


struct StartFilePlayingResponseBody {
    BYTE filePlayStarted;
};
using StartFilePlayingResponse = Message<0x05, 0x05, 0x07, StartFilePlayingResponseBody>;


using IsFilePlayingRequest = Message<0x05, 0x05, 0x08>;


struct IsFilePlayingResponseBody {
    BYTE filePlaying;
};
using IsFilePlayingResponse = Message<0x05, 0x05, 0x08, IsFilePlayingResponseBody>;


struct ThermistorValuesMessageBody {
    WordValue tempA;
    WordValue tempB;
    WordValue tempC;
    WordValue tempD;
};
using ThermistorValuesMessage = Message<0x05, 0x05, 0x80, ThermistorValuesMessageBody>;


END_NAMESPACE_MW_BLACKROCK_LEDDRIVER


#endif /* !defined(BlackrockLEDDriver_BlackrockLEDDriverCommand_h) */


























