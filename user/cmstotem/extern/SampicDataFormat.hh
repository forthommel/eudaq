#ifndef SampicDataFormat_hh
#define SampicDataFormat_hh

#include <array>

namespace sampic {
  template<typename T> static T grayDecode(T gray) {
    T bin = gray;
    while (gray >>= 1)
      bin ^= gray;
    return bin;
  }

  struct EventHeader : std::array<uint16_t,12> {
    static constexpr uint16_t m_event_begin = 0xebeb;
    bool valid() const { return at(0) == m_event_begin; }
    uint8_t boardId() const { return (at(1) >> 13) & 0x7; }
    uint8_t sampicId() const { return (at(1) >> 12) & 0x1; }
    uint64_t sf2Timestamp() const { return (at(2) & 0xffff)+((at(3) & 0xffff) << 16)+(at(4) & 0xf)*(1ull << 32); }
    uint32_t eventNumber() const { return (at(5) & 0xffff)+((at(6) & 0xffff) << 16); }
    uint32_t triggerNumber() const { return (at(7) & 0xffff)+((at(8) & 0xffff) << 16); }
    uint16_t activeChannels() const { return at(9) & 0xffff; }
    uint8_t readoutOffset() const { return (at(10) >> 8) & 0xff; }
    uint8_t sampleNumber() const { return at(10) & 0xff; }
    uint8_t fwVersion() const { return (at(11) >> 8) & 0xff; }
    uint8_t clockMonitor() const { return at(11) & 0xff; }
  };

  struct EventTrailer : std::array<uint16_t,1> {
    static constexpr uint16_t m_event_end = 0xeeee;
    bool valid() { return at(0) == m_event_end; }
  };

  struct LpbusHeader : std::array<uint16_t,2> {
    uint16_t payload() const { return ((at(0) >> 8) & 0xff)+(at(1) & 0xff); }
    uint8_t channelIndex() const { return at(0) & 0xff; }
    uint8_t command() const { return (at(1) >> 8) & 0xff; }
  };

  struct SampicHeader : std::array<uint16_t,7> {
    bool valid() const { return (at(0) & 0xff) == 0x69; }
    uint8_t adcLatch() const { return (at(0) >> 8) & 0xff; }
    uint64_t fpgaTimestamp() const { return ((at(1) >> 8) & 0xff)+((at(2) & 0xffff) << 8)+((at(3) & 0xffff) << 24); }
    uint16_t sampicTimestampAGray() const { return at(4) & 0xffff; }
    uint16_t sampicTimestampBGray() const { return at(5) & 0xffff; }
    uint16_t sampicTimeStampA() const { return grayDecode<uint16_t>(sampicTimestampAGray()); }
    uint16_t sampicTimeStampB() const { return grayDecode<uint16_t>(sampicTimestampBGray()); }
    uint16_t cellInfo() const { return at(6) & 0xffff; }
    uint8_t channelIndex() const { return (cellInfo() >> 6) & 0xf; }
    uint8_t sampicCellInfo() const { return cellInfo() & 0x3f; }
  };

  template<size_t N>
  struct SampicStream{
    SampicHeader header;
    std::array<uint16_t,N> samples;
  };

  template<size_t N>
  struct ChannelStream{
    LpbusHeader header;
    SampicStream<N> sampic;
  };
}

#endif

