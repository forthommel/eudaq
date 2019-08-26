#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

#include "SampicEvent.hh"

class SampicRawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("SampicRaw");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<SampicRawEvent2StdEventConverter>(SampicRawEvent2StdEventConverter::m_id_factory);
}

bool SampicRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  d2.reset(new eudaq::SampicEvent(*ev));
  return true;
}
