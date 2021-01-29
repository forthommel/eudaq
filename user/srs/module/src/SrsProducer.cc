#include "eudaq/Producer.hh"

#include <mutex>
#include <map>
#include <set>

#include "srsdriver/SlowControl.h"
#include "srsdriver/Messenger.h"
#include "srsdriver/Receiver.h"

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
  std::unique_ptr<srs::SlowControl> m_srs;
  bool m_debug;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<SrsProducer, const std::string&, const std::string&>
    (SrsProducer::m_id_factory);
}

SrsProducer::SrsProducer(const std::string &name, const std::string &runcontrol):
  Producer(name, runcontrol),m_ts_bore(0),m_exit_of_run(false), m_debug(false){
}

void SrsProducer::DoInitialise(){
  auto ini = GetInitConfiguration();
  m_debug = ini->Get("SRS_DEBUG", 0);
  const std::string in_scripts = ini->Get("SRS_INIT_SCRIPTS", "");
  if (in_scripts.empty())
    EUDAQ_THROW("Failed to retrieve an initialisation script!");

  std::string addr;
  srs::port_t port;
  for (const auto& ini_file : eudaq::split(in_scripts, ",")) {
    const auto config = srs::Messenger::parseCommands(ini_file, addr, port);
    srs::Messenger msg(addr, m_debug);
    msg.send(port, config);
  }
}

void SrsProducer::DoConfigure(){
  auto cfg = GetConfiguration();
  const std::string addr_server = cfg->Get("SRS_SERVER_ADDR", "10.0.0.2");
  if (addr_server.empty())
    EUDAQ_THROW("Failed to retrieve the SRS server address!");

  m_srs = std::make_unique<srs::SlowControl>(addr_server, m_debug);
}

void SrsProducer::DoStartRun(){
  m_srs->setReadoutEnable(true);
  m_exit_of_run = false;
}

void SrsProducer::DoStopRun(){
  m_srs->setReadoutEnable(false);
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
    m_srs->read(0);
  }
}
