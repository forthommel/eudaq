#include "eudaq/ROOTMonitor.hh"
#include "eudaq/TTreeEventConverter.hh"

#include "TString.h"

class TTreeEventMonitor : public eudaq::ROOTMonitor {
public:
  TTreeEventMonitor(const std::string & name, const std::string & runcontrol);
  void AtConfiguration() override;
  void AtRunStop() override;
  void AtEventReception(eudaq::EventSP ev) override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("TTreeEventMonitor");

private:
  // configuration flags
  bool m_en_print = false;

  // monitored variables
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Monitor>::
    Register<TTreeEventMonitor, const std::string&, const std::string&>(TTreeEventMonitor::m_id_factory);
}

TTreeEventMonitor::TTreeEventMonitor(const std::string & name, const std::string & runcontrol)
  :ROOTMonitor(name, "TTreeEvent monitor", runcontrol){
}

void TTreeEventMonitor::AtConfiguration(){
  auto conf = GetConfiguration();
  if (conf) {
    m_en_print = conf->Get("ENABLE_PRINT", 1);
  }

  // book all monitoring elements
}

void TTreeEventMonitor::AtEventReception(eudaq::EventSP ev){
  if (m_en_print)
    ev->Print(std::cout);
  auto ttev = std::make_shared<eudaq::TTreeEvent>();
  eudaq::TTreeEventConverter::Convert(ev, ttev, GetConfiguration());
}

void TTreeEventMonitor::AtRunStop(){
  m_monitor->SaveFile(Form("/tmp/ttree_monitor_run%04u.root", GetRunNumber()));
}

