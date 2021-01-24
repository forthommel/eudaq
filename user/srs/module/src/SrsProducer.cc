#include "eudaq/Producer.hh"

#include <mutex>
#include <map>
#include <set>

#include "srsdriver/SlowControl.h"

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
  std::unique_ptr<srs::SlowControl> srs_;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<SrsProducer, const std::string&, const std::string&>
    (SrsProducer::m_id_factory);
}

SrsProducer::SrsProducer(const std::string &name, const std::string &runcontrol):
  Producer(name, runcontrol),m_ts_bore(0),m_exit_of_run(false){
}

void SrsProducer::DoInitialise(){
  auto ini = GetInitConfiguration();
  const int debug = ini->Get("SRS_DEBUG", 1);
  const std::string in_scripts = ini->Get("SRS_INIT_SCRIPTS", "");
  if (in_scripts.empty())
    EUDAQ_THROW("Failed to retrieve an initialisation script!");
  const std::string addr_server = ini->Get("SRS_SERVER_ADDR", "10.0.0.2");
  if (addr_server.empty())
    EUDAQ_THROW("Failed to retrieve the SRS server address!");

  srs::SlowControl(addr_server, debug);

  std::string addr;
  srs::port_t port;
  for (const auto& ini_file : eudaq::split(in_scripts, ",")) {
    const auto config = srs::SlowControl::parseCommands(ini_file, addr, port);
    srs::Messenger msg(addr, debug);
    msg.send(port, config);
  }
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
