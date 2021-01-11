#include "eudaq/Producer.hh"

extern "C"{
#include "srsLib.h"
}

#include <mutex>
#include <map>
#include <set>

class SrsProducer:public eudaq::Producer{
public:
  SrsProducer(const std::string &name, const std::string &runcontrol);

  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoReset() override;
  void DoTerminate() override;
  void RunLoop() override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("SrsProducer");
private:
  uint64_t m_ts_bore;
  bool m_exit_of_run;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<SrsProducer, const std::string&, const std::string&>
    (SrsProducer::m_id_factory);
}

SrsProducer::SrsProducer(const std::string &name, const std::string &runcontrol):
  Producer(name, runcontrol),m_ts_bore(0),m_exit_of_run(false){
  srsSetDebugMode(1);
}

void SrsProducer::DoInitialise(){
  auto ini = GetInitConfiguration();
  const std::string ip_server = ini->Get("SRS_SERVER_ADDR", "10.0.0.2");
  if (ip_server.empty())
    EUDAQ_THROW("Invalid SRS server address!");

  srsStatus((char*)ip_server.c_str(), 0);
}

void SrsProducer::DoConfigure(){
}

void SrsProducer::DoStartRun(){
  m_exit_of_run = false;
}

void SrsProducer::DoStopRun(){
  m_exit_of_run = true;
}

void SrsProducer::DoReset(){
  m_exit_of_run = true;
  //...
}

void SrsProducer::DoTerminate(){
  m_exit_of_run = true;
}

void SrsProducer::RunLoop(){
  auto tp_start_run = std::chrono::steady_clock::now();
  std::map<unsigned int,eudaq::EventUP> map_events;

  while (!m_exit_of_run) {
  }
}
