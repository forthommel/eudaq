#include "eudaq/ROOTMonitor.hh"
#include "eudaq/StdEventConverter.hh"

#include "srsdriver/SlowControl.h"
#include "srsdriver/Messenger.h"
#include "srsdriver/Receiver.h"
#include "srsreadout/AdcData.h"

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
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Monitor>::
    Register<SrsMonitor, const std::string&, const std::string&>(SrsMonitor::m_id_factory);
}

SrsMonitor::SrsMonitor(const std::string & name, const std::string & runcontrol)
  :ROOTMonitor(name, "SRS monitor", runcontrol){
}

void SrsMonitor::AtConfiguration(){
}

void SrsMonitor::AtRunStop(){
}

void SrsMonitor::AtEventReception(eudaq::EventSP ev){
  if (!m_config) {
    if (ev->GetDescription() != "SrsConfig")
      EUDAQ_THROW("Failed to retrieve the SRS configuration from data stream!");
    m_config = std::make_unique<eudaq::SrsConfig>(*ev);
  }

  eudaq::SrsEventSP event;
  auto eb_mode = srs::ApvAppRegister::FRAME_EVENT_CNT; //FIXME
  if (ev->GetSubEvents().empty() && ev->GetDescription() == "SrsRaw")
    event = std::make_shared<eudaq::SrsEvent>(*ev, eb_mode);
  for (auto& sub_evt : ev->GetSubEvents()) {
    if (sub_evt->GetDescription() == "SrsRaw")
      event = std::make_shared<eudaq::SrsEvent>(*ev, eb_mode);
  }
  if (!event)
    return;

  event->Print(std::cout);
  /*srs::SrsData* data = nullptr;
  while (data = event->Data()->nextPtr())
    data->print(std::cout);*/
  if (event->Data()->dataSource() == srs::SrsFrame::ADC) {
    const auto adc_frm = event->Data()->next<srs::AdcData>();
    adc_frm.print(std::cout);
  }
}
