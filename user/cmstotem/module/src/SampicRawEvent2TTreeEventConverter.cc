#include "eudaq/TTreeEventConverter.hh"
#include "eudaq/RawEvent.hh"

#include "SampicEvent.hh"

class SampicRawEvent2TTreeEventConverter: public eudaq::TTreeEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::TTreeEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("SampicRaw");
private:
  template<typename T> void RegisterVariable(eudaq::TTreeEventSP&, const char*, T*, const char*) const;
  //mutable bool m_first = true;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::TTreeEventConverter>::
    Register<SampicRawEvent2TTreeEventConverter>(SampicRawEvent2TTreeEventConverter::m_id_factory);
}

template<typename T>
void SampicRawEvent2TTreeEventConverter::RegisterVariable(eudaq::TTreeEventSP& ev, const char* name, T* var, const char* leaflist) const{
//std::cout << ev->GetListOfBranches()->Size() << std::endl;
//ev->Print();
  if (ev->GetListOfBranches()->FindObject(name)) {
    int status = ev->SetBranchAddress(name, var);
    if (status != 0)
      EUDAQ_THROW("Invalid SetBranchAddress status, returned "+std::to_string(status));
  }
  else
    if (ev->Branch(name, var, leaflist) == nullptr)
      EUDAQ_THROW("Failed to add a new branch to output tree");
}

bool SampicRawEvent2TTreeEventConverter::Converting(eudaq::EventSPC d1, eudaq::TTreeEventSP d2, eudaq::ConfigSPC conf) const{
  //std::cout << __PRETTY_FUNCTION__ << std::endl;
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  ev->Print(std::cout);
  //if (m_first) {
    /*d2->Branch("x_pixel", &m_x_pixel, "x_pixel/I");
    d2->Branch("samples", &m_samples, "samples/I");
    d2->Branch("max_ampl", m_max_ampl, "max_ampl[samples]/I");*/
    //m_first = false;
  //}
  /*if (!d2->IsFlagPacket()) {
    d2->SetFlag(d1->GetFlag());
    d2->SetRunN(d1->GetRunN());
    d2->SetEventN(d1->GetEventN());
    d2->SetStreamN(d1->GetStreamN());
    d2->SetTriggerN(d1->GetTriggerN(), d1->IsFlagTrigger());
    d2->SetTimestamp(d1->GetTimestampBegin(), d1->GetTimestampEnd(), d1->IsFlagTimestamp());
  }*/

  /*uint32_t id = ev->GetExtendWord();
  std::cout << "ID " << id << std::endl;*/
  std::cout << "ev =" << d1->GetEventN() << std::endl;
  //ev->AddSubEvent(d1);
  std::cout << "nsub =" << d1->GetNumSubEvent() << std::endl;
  auto event = std::make_shared<eudaq::SampicEvent>(*ev);
  float x_pixel, samples, max_ampl[100];
  //event->Print(std::cout);
  /*for (const auto& smp : *event) {
    const auto& sampic_info = smp.sampic;
    std::cout << sampic_info.header.fpgaTimestamp() << std::endl;
    for (const auto& sample : sampic_info.samples) std::cout << ">>" << sample;
    std::cout << std::endl;
  }*/
    /*uint8_t y_pixel = block[1];
    std::vector<uint8_t> hit(block.begin()+2, block.end());
    std::vector<uint8_t> hitxv;
    if(hit.size() != x_pixel*y_pixel)
      EUDAQ_THROW("Unknown data");
    TString temp = "block";*/

    x_pixel = 42;
    samples = 10;
    for (int i = 0; i < samples; ++i) max_ampl[i] = 1+i;


    std::cout << x_pixel << "|" << samples << "|" << max_ampl[0] << std::endl;

  /*if (d2->GetListOfBranches()->FindObject("x_pixel"))
    d2->SetBranchAddress("x_pixel", &x_pixel);
  else
    d2->Branch("x_pixel", &x_pixel, "x_pixel/b");*/
    RegisterVariable<float>(d2, "x_pixel", &x_pixel, "x_pixel/F");
    RegisterVariable<float>(d2, "samples", &samples, "samples/F");
    RegisterVariable<float>(d2, "max_ampl", max_ampl, "max_ampl[samples]/F");
    d2->Fill();
  return true;
}
