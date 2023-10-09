#include "eudaq/ROOTMonitor.hh"
#include "eudaq/StdEventConverter.hh"

#include <srsdriver/Messenger.h>
#include <srsdriver/Receiver.h>
#include <srsdriver/SlowControl.h>
#include <srsreadout/AdcData.h>
#include <srsutils/Logging.h>

#include "SrsConfig.hh"
#include "SrsEvent.hh"

#include <TGraph.h>
#include <TH1.h>

#include <random> // for std::accumulate

class SrsMonitor : public eudaq::ROOTMonitor {
public:
  explicit SrsMonitor(const std::string &name, const std::string &runcontrol)
      : ROOTMonitor(name, "SRS monitor", runcontrol) {}

  void AtConfiguration() override;
  void AtRunStop() override {}
  void AtEventReception(eudaq::EventSP ev) override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("SrsMonitor");

private:
  static const uint32_t m_srs_hash = eudaq::cstr2hash("SrsRaw");
  void parseEvent(const std::shared_ptr<eudaq::SrsEvent> &event);
  eudaq::SrsConfigUP m_config;
  srs::SystemRegister m_sys;
  srs::ApvAppRegister m_apvapp;
  std::map<unsigned short, TGraph *> m_map_ch_framebuf, m_map_ch_framebuf_zs;
  std::map<unsigned short, TH1D *> m_map_ch_maxampl, m_map_ch_maxampl_zs;
  unsigned int m_num_samples_baseline{20};
};

namespace {
  auto dummy0 =
      eudaq::Factory<eudaq::Monitor>::Register<SrsMonitor, const std::string &,
                                               const std::string &>(
          SrsMonitor::m_id_factory);
}

void SrsMonitor::AtConfiguration() {
  if (auto cfg = GetConfiguration()) {
    // retrieve run configuration directly for SRS
    // (as a fallback if not retrieved from data stream)
    const std::string addr_server = cfg->Get("SRS_SERVER_ADDR", "10.0.0.2");
    if (addr_server.empty())
      EUDAQ_THROW("Failed to retrieve the SRS server address!");
    try {
      srs::SlowControl sc(addr_server);
      m_sys = sc.readSystemRegister();
      m_apvapp = sc.readApvAppRegister();
    } catch (const srs::LogMessage &) {
    }
    m_num_samples_baseline = cfg->Get("SRS_NUM_SAMPLES_BASELINE", 20);
  }
}

void SrsMonitor::AtEventReception(eudaq::EventSP ev) {
  if (ev->GetSubEvents().empty()) { // standard event with no sub-event
    if (ev->GetDescription() == "SrsConfig") {
      m_config = std::make_unique<eudaq::SrsConfig>(*ev);
      m_sys = m_config->SystemRegister();
      m_apvapp = m_config->ApvAppRegister();
    } else if (ev->GetDescription() == "SrsRaw")
      parseEvent(std::make_shared<eudaq::SrsEvent>(*ev, m_apvapp.ebMode()));
  } else // in case sub-events are present in event stream
    for (auto &sub_evt : ev->GetSubEvents())
      if (sub_evt->GetDescription() == "SrsRaw")
        parseEvent(std::make_shared<eudaq::SrsEvent>(*ev, m_apvapp.ebMode()));
}

void SrsMonitor::parseEvent(const std::shared_ptr<eudaq::SrsEvent> &event) {
  // event->Print(std::cout);
  for (size_t frame_id = 0; frame_id < event->NumFrames(); ++frame_id) {
    const auto *frame = event->Data(frame_id);
    if (frame->dataSource() != srs::SrsFrame::ADC)
      return;
    const auto ch_id = frame->daqChannel();
    if (m_map_ch_framebuf.count(ch_id) == 0) {
      m_map_ch_framebuf[ch_id] = m_monitor->Book<TGraph>(
          Form("channel %zu/framebuf", ch_id), "Frame buffer");
      m_monitor->SetPersistant(m_map_ch_framebuf[ch_id], false);
      m_map_ch_framebuf[ch_id]->SetTitle(";Time slice;ADC count");
      m_map_ch_framebuf_zs[ch_id] = m_monitor->Book<TGraph>(
          Form("channel %zu/framebuf_zs", ch_id), "Frame buffer (ZS)");
      m_monitor->SetPersistant(m_map_ch_framebuf_zs[ch_id], false);
      m_map_ch_framebuf_zs[ch_id]->SetTitle(
          ";Time slice;ADC count (zero-suppressed)");
      m_map_ch_maxampl[ch_id] = m_monitor->Book<TH1D>(
          Form("channel %zu/maxampl", ch_id), "Max.amplitude",
          Form("maxampl_ch%zu", ch_id),
          Form("Channel %zu;Max. amplitude;Events", ch_id), 100, 0., 4000.);
      m_map_ch_maxampl_zs[ch_id] = m_monitor->Book<TH1D>(
          Form("channel %zu/maxampl_zs", ch_id), "Max.amplitude (ZS)",
          Form("maxampl_ch%zu_zs", ch_id),
          Form("Channel %zu;Max. amplitude (zero-suppressed);Events", ch_id),
          100, 0., 2000.);
    }
    const auto adc_frm = frame->next<srs::AdcData>();
    int i = 0;
    float max_ampl = -1., max_ampl_zs = -1.;
    while (true) {
      try {
        const auto frms = adc_frm.next();
        float baseline =
            std::accumulate(frms.data.begin(),
                            frms.data.begin() + m_num_samples_baseline, 0.) *
            1. / m_num_samples_baseline;
        for (const auto &frm : frms.data) {
          const float val = frm, val_zs = val - baseline;
          m_map_ch_framebuf[ch_id]->SetPoint(m_map_ch_framebuf[ch_id]->GetN(),
                                             i, val);
          m_map_ch_framebuf_zs[ch_id]->SetPoint(
              m_map_ch_framebuf_zs[ch_id]->GetN(), i, val_zs);
          max_ampl = std::max(max_ampl, val);
          max_ampl_zs = std::max(max_ampl_zs, val_zs);
          ++i;
        }
        m_map_ch_maxampl[ch_id]->Fill(max_ampl);
        m_map_ch_maxampl_zs[ch_id]->Fill(max_ampl_zs);
      } catch (const srs::SrsFrame::NoMoreFramesException &) {
        break;
      }
    }
  }
}
