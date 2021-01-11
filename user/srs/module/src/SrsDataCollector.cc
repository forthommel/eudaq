#include "eudaq/DataCollector.hh"
#include <mutex>
#include <deque>
#include <map>
#include <set>

class SrsDataCollector:public eudaq::DataCollector{
public:
  SrsDataCollector(const std::string &name,
		   const std::string &runcontrol);

  void DoStartRun() override;
  void DoConfigure() override;
  void DoConnect(eudaq::ConnectionSPC id) override;
  void DoDisconnect(eudaq::ConnectionSPC id) override;
  void DoReceive(eudaq::ConnectionSPC id, eudaq::EventSP ev) override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("SrsDataCollector");
private:
  uint64_t m_ts_bore_bif;
  uint64_t m_ts_bore_cal;
  std::deque<eudaq::EventSPC> m_que_bif;
  std::deque<eudaq::EventSPC> m_que_cal;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::DataCollector>::
    Register<SrsDataCollector, const std::string&, const std::string&>
    (SrsDataCollector::m_id_factory);
}

SrsDataCollector::SrsDataCollector(const std::string &name, const std::string &runcontrol):
  DataCollector(name, runcontrol),m_ts_bore_bif(0),m_ts_bore_cal(0){
}

void SrsDataCollector::DoConnect(eudaq::ConnectionSPC id){
  std::string name = id->GetName();
  if(name != "srsProducer" && name != "Srs1")
    EUDAQ_WARN("unsupported producer is connecting.");
}

void SrsDataCollector::DoDisconnect(eudaq::ConnectionSPC id){
}

void SrsDataCollector::DoStartRun(){
  m_que_bif.clear();
  m_que_cal.clear();
}

void SrsDataCollector::DoConfigure(){
  //Nothing to do
}

void SrsDataCollector::DoReceive(eudaq::ConnectionSPC id, eudaq::EventSP ev){
  if(id->GetName() == "caliceahcalbifProducer"){
    if(ev->IsBORE())
      m_ts_bore_bif = ev->GetTag("ROCStartTS", m_ts_bore_bif);
    m_que_bif.push_back(std::move(ev));
  }
  else if(id->GetName() == "Srs1"){
    if(ev->IsBORE())
      m_ts_bore_cal = ev->GetTag("ROCStartTS", m_ts_bore_cal);
    m_que_cal.push_back(ev);
  }
  else{
    EUDAQ_WARN("Event from unknown producer");
  }
  while(!m_que_bif.empty() && !m_que_cal.empty()){
    auto ev_bif = m_que_bif.front();
    auto ev_cal = m_que_cal.front();
    if(ev_bif->GetTriggerN() > ev_cal->GetTriggerN()){
      EUDAQ_WARN("ev_bif->GetTriggerN() > ev_cal->GetTriggerN()");
      m_que_cal.pop_front();
      continue;
    }
    else if(ev_bif->GetTriggerN() < ev_cal->GetTriggerN()){
      EUDAQ_WARN("ev_bif->GetTriggerN() < ev_cal->GetTriggerN()");
      m_que_bif.pop_front();
      continue;
    }
    auto ev_wrap =  eudaq::Event::MakeUnique(GetFullName());
    //TODO:: compare the timestamp
    uint64_t ts_beg = ev_bif->GetTimestampBegin();
    uint64_t ts_end = ev_bif->GetTimestampEnd();
    ev_wrap->AddSubEvent(ev_bif);
    ev_wrap->AddSubEvent(ev_cal);
    ev_wrap->SetTimestamp(ts_beg, ts_end, false);
    ev_wrap->SetTriggerN(ev_bif->GetTriggerN());
    // ev_wrap->SetBORE();
    // ev_wrap->SetEORE();
    m_que_bif.pop_front();
    m_que_cal.pop_front();
    WriteEvent(std::move(ev_wrap));
  }
  if(m_que_bif.size() > 1000 || m_que_cal.size() > 1000){
    EUDAQ_WARN("m_que_bif.size() > 1000 || m_que_cal.size() > 1000");
  }
}
