#ifndef SampicEvent_hh
#define SampicEvent_hh

#include "eudaq/StandardEvent.hh"
#include "SampicDataFormat.hh"

namespace eudaq {
  class SampicEvent;
  using SampicEventUP = Factory<SampicEvent>::UP;
  using SampicEventSP = Factory<SampicEvent>::SP;
  using SampicEventSPC = Factory<SampicEvent>::SPC;

  class DLLEXPORT SampicEvent : public StandardEvent {
  public:
    SampicEvent();
    SampicEvent(const Event&);
    SampicEvent(Deserializer&);

    void Serialize(Serializer&) const override;
    void ConvertBlock(const std::vector<uint8_t>&);
    void Print(std::ostream & os,size_t offset = 0) const override;

    static SampicEventSP MakeShared();
    static const uint32_t m_id_factory = cstr2hash("SampicEvent");

  private:
    sampic::EventHeader m_header;
    std::vector<sampic::ChannelStream<64> > m_ch_stream;
    sampic::EventTrailer m_trailer;
  };
}

#endif

