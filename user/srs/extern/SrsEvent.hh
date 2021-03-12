#ifndef SrsEvent_hh
#define SrsEvent_hh

#include "eudaq/StandardEvent.hh"
#include "srsreadout/SrsFrame.h"

namespace eudaq {
  class SrsEvent;
  using SrsEventUP = Factory<SrsEvent>::UP;
  using SrsEventSP = Factory<SrsEvent>::SP;
  using SrsEventSPC = Factory<SrsEvent>::SPC;

  class DLLEXPORT SrsEvent : public StandardEvent {
  public:
    SrsEvent();
    SrsEvent(const Event&);

    void ConvertBlock(const std::vector<uint8_t>&);
    void Print(std::ostream&, size_t offset = 0) const override;

    static SrsEventSP MakeShared();
    static const uint32_t m_id_factory = cstr2hash("SrsEvent");
  };
}

#endif
