#include "eudaq/ROOTMonitor.hh"
#include "eudaq/StdEventConverter.hh"

#include "srsdriver/SrsFrame.h"

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
}
