#include "eudaq/DataCollector.hh"

#include "sampicdaq/acq.h"
#include "SampicDataFormat.hh"

#include <mutex>
#include <deque>
#include <map>
#include <set>

class SampicDataCollector : public eudaq::DataCollector{
public:
  SampicDataCollector(const std::string &name, const std::string &rc);
  void DoConnect(eudaq::ConnectionSPC id) override;
  void DoDisconnect(eudaq::ConnectionSPC id) override;
  void DoConfigure() override;
  void DoReset() override;
  void DoReceive(eudaq::ConnectionSPC id, eudaq::EventSP ev) override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("SampicDataCollector");

private:
  std::mutex m_mtx_map;
  std::map<eudaq::ConnectionSPC, std::deque<eudaq::EventSPC>> m_conn_evque;
  std::set<eudaq::ConnectionSPC> m_conn_inactive;

  uint32_t m_print_hdr = 0;
  uint32_t m_print_evt = 0;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::DataCollector>::
    Register<SampicDataCollector, const std::string&, const std::string&>
    (SampicDataCollector::m_id_factory);
}

SampicDataCollector::SampicDataCollector(const std::string &name, const std::string &rc):
  DataCollector(name, rc){
}

void SampicDataCollector::DoConnect(eudaq::ConnectionSPC idx){
  std::unique_lock<std::mutex> lk(m_mtx_map); // a bit of thread safety...
  m_conn_evque[idx].clear();
  m_conn_inactive.erase(idx);
}

void SampicDataCollector::DoDisconnect(eudaq::ConnectionSPC idx){
  std::unique_lock<std::mutex> lk(m_mtx_map); // a bit of thread safety...
  m_conn_inactive.insert(idx);
  if (m_conn_inactive.size() == m_conn_evque.size()){
    m_conn_inactive.clear();
    m_conn_evque.clear();
  }
}

void SampicDataCollector::DoConfigure(){
  m_print_hdr = 0;
  m_print_evt = 0;
  auto conf = GetConfiguration();
  if (conf) {
    conf->Print();
    m_print_hdr = conf->Get("SAMPIC_PRINT_HEADERS", 0);
    m_print_evt = conf->Get("SAMPIC_PRINT_EVENTS", 0);
  }
}

void SampicDataCollector::DoReset(){
  std::unique_lock<std::mutex> lk(m_mtx_map); // a bit of thread safety...
  m_print_hdr = 0;
  m_print_evt = 0;
  m_conn_evque.clear();
  m_conn_inactive.clear();
}

void SampicDataCollector::DoReceive(eudaq::ConnectionSPC idx, eudaq::EventSP evsp){
  std::unique_lock<std::mutex> lk(m_mtx_map); // a bit of thread safety...
  if (!evsp->IsFlagTrigger()){
    EUDAQ_THROW("!evsp->IsFlagTrigger()");
  }

  //--- first receive the uncorrected stream
  m_conn_evque[idx].push_back(evsp);
  for (auto& conn_evque: m_conn_evque) {
    if (conn_evque.second.empty())
      return;
  }

  //--- build new event
  auto ev_sync = eudaq::Event::MakeUnique("SampicEvent");

  if (m_print_evt)
    ev_sync->Print(std::cout);

  WriteEvent(std::move(ev_sync));
}
