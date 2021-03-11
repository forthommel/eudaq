#include "eudaq/Producer.hh"

#include <mutex>
#include <map>
#include <set>
#include <streambuf>
#include <future>
#include <atomic>

#include "srsdriver/SlowControl.h"
#include "srsdriver/Messenger.h"
#include "srsdriver/Receiver.h"
#include "srsdriver/SrsFrame.h"

#include "srsutils/Logging.h"

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
  uint64_t m_ts_bore = 0;
  std::atomic_bool m_running = {false};
  unsigned long long m_trig_num = 0;

  /// Output stream derivation to EUDAQ_INFO
  class SrsBuffer:public std::ostream{
    private:
      struct SrsLogger:public std::stringbuf{
        int sync() override{
          int ret = std::stringbuf::sync();
          EUDAQ_DEBUG(str());
          str("");
          return ret;
        }
      } buff_;
    public:
      SrsBuffer():buff_(), std::ostream(&buff_){}
  } m_ostream;
  std::unique_ptr<srs::SlowControl> m_srs;
  std::vector<srs::port_t> m_rd_ports;
  std::vector<std::vector<srs::SrsFrameCollection> > m_frames;

  using Frames = std::vector<srs::SrsFrame>;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<SrsProducer, const std::string&, const std::string&>
    (SrsProducer::m_id_factory);
}

SrsProducer::SrsProducer(const std::string &name, const std::string &runcontrol):
  Producer(name, runcontrol){
  srs::Logger::get().setOutput(&m_ostream, false);
}

void SrsProducer::DoInitialise(){
  auto ini = GetInitConfiguration();

  // set debugging mode
  if (ini->Get("SRS_DEBUG", 0) == 1)
    srs::Logger::get().setLevel(srs::Logger::Level::debug);

  // set the list of initialisation scripts
  const std::string in_scripts = ini->Get("SRS_INIT_SCRIPTS", "");
  if (in_scripts.empty())
    EUDAQ_THROW("Failed to retrieve an initialisation script!");

  for (const auto& ini_file : eudaq::split(in_scripts, ",")) {
    std::string addr;
    srs::port_t port;
    EUDAQ_DEBUG("Parsing and sending SRS configuration commands in "+ini_file);
    const auto config = srs::Messenger::parseCommands(ini_file, addr, port);
    srs::Messenger msg(addr);
    msg.send(port, config);
  }
  for (const auto& port : eudaq::split(ini->Get("SRS_READOUT_PORTS", "6006")))
    m_rd_ports.emplace_back(std::stoi(port));
}

void SrsProducer::DoConfigure(){
  auto cfg = GetConfiguration();
  const std::string addr_server = cfg->Get("SRS_SERVER_ADDR", "10.0.0.2");
  if (addr_server.empty())
    EUDAQ_THROW("Failed to retrieve the SRS server address!");

  m_srs = std::make_unique<srs::SlowControl>(addr_server);
  for (const auto& port : m_rd_ports) {
    m_srs->addFec(port);
    m_frames.emplace_back(); // add a new collection of frames
  }
}

void SrsProducer::DoStartRun(){
  m_srs->setReadoutEnable(true);
  m_running = true;
  std::cout<<"Finished dostartrun"<<std::endl;
}

void SrsProducer::DoStopRun(){
  m_srs->setReadoutEnable(false);
  m_running = false;
  std::cout<<"Finished dostoprun"<<std::endl;
}

void SrsProducer::DoReset(){
  m_running = false;
  //...
}

void SrsProducer::DoTerminate(){
  m_running = false;
}

void SrsProducer::RunLoop(){
  auto tp_start_run = std::chrono::steady_clock::now();
  std::map<unsigned int,eudaq::EventUP> map_events; // trigger time -> event

  for (size_t i = 0; i < m_srs->numFec(); ++i) {
    auto future = std::async(std::launch::async, &srs::SlowControl::readout, m_srs.get(), std::ref(m_frames[i]), i, std::ref(m_running));
    if (future.wait_for(std::chrono::milliseconds(500)) == std::future_status::timeout) {
      m_running = false;
      return;
    }
  }
  for (size_t i = 0; i < m_frames.size(); ++i) { // loop over all FECs
    for (auto& frames : m_frames.at(i)) { // loop over all events in FEC
      const auto trig_time_beg = frames.begin()->frameCounter().timestamp();
      const auto trig_time_end = frames.rbegin()->frameCounter().timestamp();
      auto& ev = map_events[trig_time_beg];
      if (!ev) {
        // create a new output event if not already found
        ev = eudaq::Event::MakeUnique("SrsRaw");
        ev->SetTriggerN(m_trig_num);
        ev->SetEventN(m_trig_num);
        ev->SetTimestamp(trig_time_beg, trig_time_end);
      }
      for (const auto& frame : frames) // loop over all frames in event
        ev->AddBlock(frame.daqChannel(), frame);
      frames.clear();
    }
  }
  for (auto it = map_events.begin(); it != map_events.end();) { // no increment
    SendEvent(std::move(it->second));
    it = map_events.erase(it);
    if (m_trig_num++ % 1000 == 0)
      EUDAQ_INFO("Number of triggers sent: "+std::to_string(m_trig_num));
  }
  m_trig_num++;
}
