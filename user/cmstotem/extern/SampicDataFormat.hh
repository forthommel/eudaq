#ifndef SampicDataFormat_hh
#define SampicDataFormat_hh

#include <array>

namespace sampic {
  const uint16_t m_event_begin = 0xebeb;
  const uint16_t m_event_end = 0xeeee;

  template<typename T> static T grayDecode(T gray) {
    T bin = gray;
    while (gray >>= 1)
      bin ^= gray;
    return bin;
  }

  struct EventHeader{
    bool valid() const { return data[0] == m_event_begin; }
    uint8_t boardId() const { return (data[1] >> 13) & 0x7; }
    uint8_t sampicId() const { return (data[1] >> 12) & 0x1; }
    uint64_t sf2Timestamp() const { return (data[2] & 0xffff)+((data[3] & 0xffff) << 16)+(data[4] & 0xf)*(1ull << 32); }
    uint32_t eventNumber() const { return (data[5] & 0xffff)+((data[6] & 0xffff) << 16); }
    uint32_t triggerNumber() const { return (data[7] & 0xffff)+((data[8] & 0xffff) << 16); }
    uint16_t activeChannels() const { return data[9] & 0xffff; }
    uint8_t readoutOffset() const { return (data[10] >> 8) & 0xff; }
    uint8_t sampleNumber() const { return data[10] & 0xff; }
    uint8_t fwVersion() const { return (data[11] >> 8) & 0xff; }
    uint8_t clockMonitor() const { return data[11] & 0xff; }
    std::array<uint16_t,12> data;
  };

  struct LpbusHeader{
    uint16_t payload() const { return ((data[0] >> 8) & 0xff)+(data[1] & 0xff); }
    uint8_t channelIndex() const { return data[0] & 0xff; }
    uint8_t command() const { return (data[1] >> 8) & 0xff; }
    std::array<uint16_t,2> data;
  };

  struct SampicHeader{
    bool valid() const { return (data[0] & 0xff) == 0x69; }
    uint8_t adcLatch() const { return (data[0] >> 8) & 0xff; }
    uint64_t fpgaTimestamp() const { return ((data[1] >> 8) & 0xff)+((data[2] & 0xffff) << 8)+((data[3] & 0xffff) << 24); }
    uint16_t sampicTimestampAGray() const { return data[4] & 0xffff; }
    uint16_t sampicTimestampBGray() const { return data[5] & 0xffff; }
    uint16_t sampicTimeStampA() const { return grayDecode<uint16_t>(sampicTimestampAGray()); }
    uint16_t sampicTimeStampB() const { return grayDecode<uint16_t>(sampicTimestampBGray()); }
    uint16_t cellInfo() const { return data[6] & 0xffff; }
    std::array<uint16_t,7> data;
  };

  template<size_t N>
  struct SampicStream{
    SampicHeader header;
    std::array<uint16_t,N> data;
  };
  template<size_t N>
  struct ChannelStream{
    LpbusHeader header;
    SampicStream<N> sampic;
  };
  typedef ChannelStream<64> ChannelStream64;
}

#endif

