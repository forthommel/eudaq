#include "eudaq/TTreeEventConverter.hh"
#include "eudaq/RawEvent.hh"

#include "SampicEvent.hh"

class SampicRawEvent2TTreeEventConverter: public eudaq::TTreeEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::TTreeEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("SampicRaw");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::TTreeEventConverter>::
    Register<SampicRawEvent2TTreeEventConverter>(SampicRawEvent2TTreeEventConverter::m_id_factory);
}

bool SampicRawEvent2TTreeEventConverter::Converting(eudaq::EventSPC d1, eudaq::TTreeEventSP d2, eudaq::ConfigSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  /*uint32_t id = ev->GetExtendWord();
  std::cout << "ID " << id << std::endl;*/
  auto event = std::make_shared<eudaq::SampicEvent>(*ev);

  float maxAmplitude = 10.f;

  const char* temp = "maxAmplitude";
  if (d2->GetListOfBranches()->FindObject(temp))
    d2->SetBranchAddress(temp, &maxAmplitude);
  else
    d2->Branch(temp, &maxAmplitude, "maxAmplitude/F");
  return true;
}
