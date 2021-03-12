#include "eudaq/ROOTMonitor.hh"
#include "eudaq/StdEventConverter.hh"

#include "srsdriver/SlowControl.h"
#include "srsdriver/Messenger.h"
#include "srsdriver/Receiver.h"
#include "srsreadout/AdcData.h"
#include "srsutils/Logging.h"

#include "SrsEvent.hh"
#include "SrsConfig.hh"

#include "TH1.h"
#include "TH2.h"
#include "TGraph.h"

#include <random> // for std::accumulate

class SrsMonitor : public eudaq::ROOTMonitor {
public:
  SrsMonitor(const std::string & name, const std::string & runcontrol);

  void AtConfiguration() override;
  void AtRunStop() override;
  void AtEventReception(eudaq::EventSP ev) override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("SrsMonitor");

private:
  static const uint32_t m_srs_hash = eudaq::cstr2hash("SrsRaw");
  eudaq::SrsConfigUP m_config;
  srs::SystemRegister m_sys;
  srs::ApvAppRegister m_apvapp;
  std::map<unsigned short, TGraph*> m_map_ch_framebuf;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Monitor>::
    Register<SrsMonitor, const std::string&, const std::string&>(SrsMonitor::m_id_factory);
}

SrsMonitor::SrsMonitor(const std::string & name, const std::string & runcontrol)
  :ROOTMonitor(name, "SRS monitor", runcontrol){
}

void SrsMonitor::AtConfiguration(){
  { // retrieve run configuration directly for SRS
    // (as a fallback if not retrieved from data stream)
    auto cfg = GetConfiguration();
    const std::string addr_server = cfg->Get("SRS_SERVER_ADDR", "10.0.0.2");
    if (addr_server.empty())
      EUDAQ_THROW("Failed to retrieve the SRS server address!");
    try {
      srs::SlowControl sc(addr_server);
      m_sys = sc.readSystemRegister();
      m_apvapp = sc.readApvAppRegister();
    } catch (const srs::LogMessage&) {}
  }
}

void SrsMonitor::AtRunStop(){
}

void SrsMonitor::AtEventReception(eudaq::EventSP ev){
  eudaq::SrsEventSP event;

  if (ev->GetSubEvents().empty()) { // standard event with no sub-event
    if (ev->GetDescription() == "SrsConfig") {
      m_config = std::make_unique<eudaq::SrsConfig>(*ev);
      m_sys = m_config->SystemRegister();
      m_apvapp = m_config->ApvAppRegister();
    }
    else if (ev->GetDescription() == "SrsRaw")
      event = std::make_shared<eudaq::SrsEvent>(*ev, m_apvapp.ebMode());
  }
  else // in case sub-events are present in event stream
    for (auto& sub_evt : ev->GetSubEvents()) {
      if (sub_evt->GetDescription() == "SrsRaw")
        event = std::make_shared<eudaq::SrsEvent>(*ev, m_apvapp.ebMode());
    }
  if (!event)
    return;

  event->Print(std::cout);
  /*srs::SrsData* data = nullptr;
  while (data = event->Data()->nextPtr())
    data->print(std::cout);*/
  const auto ch_id = event->Data()->daqChannel();
  if (m_map_ch_framebuf.count(ch_id) == 0) {
    m_map_ch_framebuf[ch_id] = m_monitor->Book<TGraph>("framebuf_vs_ch", "Frame buffer");
    m_map_ch_framebuf[ch_id]->SetTitle(";Time slice;ADC count");
  }
  if (event->Data()->dataSource() == srs::SrsFrame::ADC) {
    const auto adc_frm = event->Data()->next<srs::AdcData>();
    adc_frm.print(std::cout);
  }
}
