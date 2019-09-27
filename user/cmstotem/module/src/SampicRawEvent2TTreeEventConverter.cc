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
  struct eventblock_t {
    uint64_t timestamp = 0;
    int event_num = 0;
    float time[64] = {0.};
    int num_samples = 0;
    int channel_id[100] = {-1};
    int fpga_timestamp[100] = {0};
  };
  struct channelblock_t {
    int num_samples = 0;
    float ampl[10][64] = {{0.}};
    float max_ampl[10] = {10.};
  };
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::TTreeEventConverter>::
    Register<SampicRawEvent2TTreeEventConverter>(SampicRawEvent2TTreeEventConverter::m_id_factory);
}

template<typename T>
void SampicRawEvent2TTreeEventConverter::RegisterVariable(eudaq::TTreeEventSP& ev, const char* name, T* var, const char* leaflist) const{
  if (ev->GetListOfBranches()->FindObject(name)) {
    int status = ev->SetBranchAddress(name, var);
    if (status != 0)
      EUDAQ_THROW("Invalid SetBranchAddress status for branch \""+std::string(name)+"\", returned "+std::to_string(status));
  }
  else
    if (ev->Branch(name, var, leaflist) == nullptr)
      EUDAQ_THROW("Failed to add a new branch \""+std::string(name)+"\" to output tree");
}

bool SampicRawEvent2TTreeEventConverter::Converting(eudaq::EventSPC d1, eudaq::TTreeEventSP d2, eudaq::ConfigSPC conf) const{
  int num_baseline = 10, num_channels = 32;
  if (conf) {
    num_baseline = conf->Get("NUM_BASELINE", 10);
    num_channels = conf->Get("NUM_CHANNELS", 32);
  }

  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  auto event = std::make_shared<eudaq::SampicEvent>(*ev);

  eventblock_t ev_block;
  { // book the event-wise block
    RegisterVariable<eventblock_t>(d2, "event", &ev_block,
      "timestamp/l:event_num/I:"
      "time[64]/F:"
      "num_samples/I:"
      "channel_id[100]/I:"
      "fpga_timestamp[100]/I");
  }
  std::vector<channelblock_t> ch_block(num_channels);
  { // book the channels-wise blocks
    size_t i = 0;
    for (auto& ch : ch_block) {
      ch.num_samples = 0;
      RegisterVariable<channelblock_t>(d2, Form("ch%zu", i++), &ch,
        "num_samples/I:"
        "ampl[10][64]/F:"
        "max_ampl[10]/F");
    }
  }

  // fill the blocks
  ev_block.timestamp = event->header().sf2Timestamp();
  ev_block.event_num = event->header().eventNumber();
  for (uint16_t i = 0; i < event->header().sampleNumber(); ++i)
    ev_block.time[i] = i*sampic::kSamplingPeriod;

  event->Print(std::cout);
  for (const auto& smp : *event) {
    const auto& sampic_info = smp.sampic;
    const unsigned short ch_id = smp.header.channelIndex();
    ev_block.channel_id[ev_block.num_samples] = ch_id;
    ev_block.fpga_timestamp[ev_block.num_samples] = sampic_info.header.fpgaTimestamp();
    const auto& samples = sampic_info.samples();
    const float baseline = 1./(num_baseline-1)*std::accumulate(
      samples.begin(),
      std::next(samples.begin(), num_baseline-1),
      0.);
    uint16_t i = 0;
    auto& num_ch_samples = ch_block[ch_id].num_samples;
    for (const auto& s_val : samples) {
      ch_block[ch_id].ampl[num_ch_samples][i] = s_val-baseline;
      if (s_val < ch_block[ch_id].max_ampl[num_ch_samples])
        ch_block[ch_id].max_ampl[num_ch_samples] = s_val;
      ++i;
    }
    ch_block[ch_id].max_ampl[num_ch_samples] -= baseline;
    num_ch_samples++;
    ev_block.num_samples++;
  }
  return true;
}
