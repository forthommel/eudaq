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
    SrsEvent(const srs::ApvAppRegister::EventBuilderMode &ebmode =
                 srs::ApvAppRegister::FRAME_EVENT_CNT);
    SrsEvent(const Event &,
             const srs::ApvAppRegister::EventBuilderMode &ebmode =
                 srs::ApvAppRegister::FRAME_EVENT_CNT);

    void ConvertBlock(const std::vector<uint8_t> &);
    void Print(std::ostream &, size_t offset = 0) const override;

    srs::SrsFrame *Data() const { return frmbuf_.get(); }

    static SrsEventSP MakeShared();
    static const uint32_t m_id_factory = cstr2hash("SrsEvent");

  private:
    srs::ApvAppRegister::EventBuilderMode eb_mode_;
    std::unique_ptr<srs::SrsFrame> frmbuf_;
  };
} // namespace eudaq

#endif
