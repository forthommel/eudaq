#include "eudaq/Producer.hh"

#include <atomic>
#include <future>
#include <map>
#include <mutex>
#include <set>
#include <streambuf>

#include "srsdriver/Messenger.h"
#include "srsdriver/Receiver.h"
#include "srsdriver/SlowControl.h"
#include "srsdriver/SrsFrame.h"
#include "srsutils/Logging.h"

#include "srsutils/Logging.h"

class SrsProducer : public eudaq::Producer {
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
  unsigned int m_timeout_sec = 0;

  /// Output stream derivation to EUDAQ_INFO
  class SrsBuffer : public std::ostream {
  private:
    struct SrsLogger : public std::stringbuf {
      int sync() override {
        int ret = std::stringbuf::sync();
        EUDAQ_DEBUG(str());
        str("");
        return ret;
      }
    } buff_;

  public:
    SrsBuffer() : buff_(), std::ostream(&buff_) {}
  } m_ostream;
  std::unique_ptr<srs::SlowControl> m_srs;
  std::vector<srs::port_t> m_rd_ports;
  eudaq::EventUP m_srs_config;
  bool m_sent_config = false;
};

namespace {
  auto dummy0 = eudaq::Factory<eudaq::Producer>::Register<
      SrsProducer, const std::string &, const std::string &>(
      SrsProducer::m_id_factory);
}

SrsProducer::SrsProducer(const std::string &name, const std::string &runcontrol)
    : Producer(name, runcontrol) {
  srs::Logger::get().setOutput(&m_ostream, false);
}

void SrsProducer::DoInitialise() {
  auto ini = GetInitConfiguration();

  // set debugging mode
  if (ini->Get("SRS_DEBUG", 0) == 1)
    srs::Logger::get().setLevel(srs::Logger::Level::debug);
  // timeout for readout
  m_timeout_sec = ini->Get("SRS_TIMEOUT_SEC", 0);

  // set the list of initialisation scripts
  const std::string in_scripts = ini->Get("SRS_INIT_SCRIPTS", "");
  if (in_scripts.empty())
    EUDAQ_THROW("Failed to retrieve an initialisation script!");

  for (const auto &ini_file : eudaq::split(in_scripts, ",")) {
    std::string addr;
    srs::port_t port;
    EUDAQ_INFO("Parsing and sending SRS configuration commands in \"" +
               ini_file + "\"");
    const auto config = srs::Messenger::parseCommands(ini_file, addr, port);
    srs::Messenger(addr).send(port, config);
  }
  for (const auto &port : eudaq::split(ini->Get("SRS_READOUT_PORTS", "6006")))
    m_rd_ports.emplace_back(std::stoi(port));
}

void SrsProducer::DoConfigure() {
  auto cfg = GetConfiguration();
  const std::string addr_server = cfg->Get("SRS_SERVER_ADDR", "10.0.0.2");
  if (addr_server.empty())
    EUDAQ_THROW("Failed to retrieve the SRS server address!");

  m_srs = std::make_unique<srs::SlowControl>(addr_server);
  for (const auto &port : m_rd_ports)
    m_srs->addFec(port);
  m_srs_config = eudaq::Event::MakeUnique("SrsConfig");
  srs::words_t sys_words, apvapp_words;
  for (const auto &word : m_srs->readSystemRegister())
    sys_words.emplace_back(*word);
  for (const auto &word : m_srs->readApvAppRegister())
    apvapp_words.emplace_back(*word);
  m_srs_config->AddBlock(0, sys_words);
  m_srs_config->AddBlock(1, apvapp_words);
  m_sent_config = false;
}

void SrsProducer::DoStartRun() {
  m_srs->setReadoutEnable(true);
  m_running = true;
}

void SrsProducer::DoStopRun() {
  m_srs->setReadoutEnable(false);
  m_running = false;
  m_sent_config = false;
}

void SrsProducer::DoReset() {
  m_running = false;
  m_sent_config = false;
  if (m_srs)
    m_srs->clearFecs();
  //...
}

void SrsProducer::DoTerminate() {
  m_running = false;
  m_sent_config = false;
  m_srs.release();
}

void SrsProducer::RunLoop() {
  // auto tp_start_run = std::chrono::steady_clock::now();
  std::map<unsigned int, eudaq::EventUP> map_events; // trigger time -> event

  while (m_running) {
    for (size_t i = 0; i < m_srs->numFec(); ++i) {
      /*auto future = std::async(std::launch::async, &srs::SlowControl::readout,
      m_srs.get(), std::ref(m_frames_colls[i]), i, std::ref(m_running)); if
      (m_timeout_sec > 0 && future.wait_for(std::chrono::seconds(m_timeout_sec))
      == std::future_status::timeout) { m_srs->setReadoutEnable(false);
        m_running = false;
        EUDAQ_THROW("Timeout reached after "+std::to_string(m_timeout_sec)+"
      sec(s).");
      }*/
      // m_srs->readout(std::ref(m_frames_colls[i]), i, std::ref(m_running));
      auto frmbuf = m_srs->read(i, m_running);
      // if (m_frames_colls[i].empty()) {
      if (frmbuf.empty()) {
        EUDAQ_DEBUG("Empty collection retrieved for FEC#" + std::to_string(i));
        continue;
      }
      const auto trig_time_beg = frmbuf.begin()->frameCounter().timestamp();
      auto &ev = map_events[trig_time_beg];
      if (!ev) {
        // create a new output event if not already found
        ev = eudaq::Event::MakeUnique("SrsRaw");
        if (!m_sent_config) {
          EUDAQ_DEBUG("SYS register wrote (" +
                      std::to_string(m_srs_config->GetBlock(0).size()) +
                      " words)");
          EUDAQ_DEBUG("APVAPP register wrote (" +
                      std::to_string(m_srs_config->GetBlock(1).size()) +
                      " words)");
          ev->AddSubEvent(std::move(m_srs_config));
          ev->SetBORE();
          // SendEvent(std::move(m_srs_config));
          m_sent_config = true;
        }
        ev->SetTriggerN(m_trig_num);
        ev->SetEventN(m_trig_num);
        const auto trig_time_end =
            frmbuf.size() > 1 ? frmbuf.rbegin()->frameCounter().timestamp() : 0;
        ev->SetTimestamp(trig_time_beg, trig_time_end);
      }
      for (const auto &buf : frmbuf)
        ev->AddBlock(buf.daqChannel(), buf);
    }
    // send all events to collector
    for (auto it = map_events.begin();
         it != map_events.end();) { // no increment
      SendEvent(std::move(it->second));
      it = map_events.erase(it);
      if (m_trig_num + 1 % 10 == 0)
        EUDAQ_INFO("Number of triggers sent: " + std::to_string(m_trig_num));
      m_trig_num++;
    }
  }
  // m_frames_colls[i].clear();
}
