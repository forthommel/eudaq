#include "eudaq/TTreeEventConverter.hh"
#include "eudaq/RawEvent.hh"

#include "SampicEvent.hh"

#include <numeric>

class SampicRawEvent2TTreeEventConverter: public eudaq::TTreeEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::TTreeEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("SampicRaw");
private:
  template<typename T> void RegisterVariable(eudaq::TTreeEventSP&, const char*, T*, const char*) const;
  //mutable bool m_first = true;
  struct eventblock_t {
    int num_samples = 0;
    int channel_id[100] = {-1};
    int fpga_timestamp[100] = {0};
  };
  struct channelblock_t {
    int num_samples = 0;
    float max_ampl[10] = {100.};
  };
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
  int num_baseline = 10;
  if (conf)
    num_baseline = conf->Get("NUM_BASELINE", 10);

  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  eventblock_t ev_block;
  channelblock_t ch_block[32];
  RegisterVariable<eventblock_t>(d2, "sampic", &ev_block, "num_samples/I:channel_id[100]/I:fpga_timestamp[100]/I");
  for (uint16_t i = 0; i < 32; ++i) {
    RegisterVariable<channelblock_t>(d2, Form("channel%d", i), &ch_block[i], "num_samples/I:max_ampl[10]/F");
  }
  std::cout << "ev =" << d1->GetEventN() << std::endl;
  auto event = std::make_shared<eudaq::SampicEvent>(*ev);
  event->Print(std::cout);
  std::cout << "id=" << (int)event->header().boardId() << std::endl;
  for (const auto& smp : *event) {
    const auto& sampic_info = smp.sampic;
    //const unsigned short ch_id = smp.header.channelIndex();
    const unsigned short ch_id = event->header().boardId()*16+sampic_info.header.channelIndex(); //FIXME use previous
    ev_block.channel_id[ev_block.num_samples] = ch_id;
    ev_block.fpga_timestamp[ev_block.num_samples] = sampic_info.header.fpgaTimestamp();
    const float baseline = std::accumulate(
      sampic_info.samples.begin(), std::next(sampic_info.samples.begin(), num_baseline-1), 0.)/(num_baseline-1);
    for (const auto& s : sampic_info.samples) {
      if (s < ch_block[ch_id].max_ampl[ev_block.num_samples])
        ch_block[ch_id].max_ampl[ev_block.num_samples] = (float)s;
    }
    ch_block[ch_id].max_ampl[ev_block.num_samples] -= baseline;
    ch_block[ch_id].num_samples++;
    ev_block.num_samples++;
  }
  d2->Fill();
  return true;
}
