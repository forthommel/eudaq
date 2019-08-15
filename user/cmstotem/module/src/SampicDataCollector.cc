#include "eudaq/DataCollector.hh"

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

  static const uint16_t m_event_begin = 0xebeb;
  static const uint16_t m_event_end = 0xeeee;

  struct EventHeader{
    bool valid() const { return data[0] == m_event_begin; }
    uint8_t boardId() const { return (data[1] >> 13) & 0x7; }
    uint8_t sampicId() const { return (data[1] >> 12) & 0x1; }
    uint64_t sf2Timestamp() const { return (data[2] & 0xffff)+((data[3] & 0xffff) << 16)+(data[4] & 0xf)*(1ull << 32); }
    uint32_t eventNumber() const { return (data[5] & 0xffff)+((data[6] & 0xffff) << 16); }
    uint32_t triggerNumber() const { return (data[7] & 0xffff)+((data[8] & 0xffff) << 16); }
    uint16_t activeChannels() const { return data[9] & 0xffff; }
    uint8_t readoutOffset() const { return (data[10] >> 8) & 0xff; }
    uint8_t sampleNumber() const { return data[10] & 0xff; }
    uint8_t fwVersion() const { return (data[11] >> 8) & 0xff; }
    uint8_t clockMonitor() const { return data[11] & 0xff; }
    std::array<uint16_t,12> data;
  };

  struct LpbusHeader{
    uint16_t payload() const { return ((data[0] >> 8) & 0xff)+(data[1] & 0xff); }
    uint8_t channelIndex() const { return data[0] & 0xff; }
    uint8_t command() const { return (data[1] >> 8) & 0xff; }
    std::array<uint16_t,2> data;
  };

  struct SampicHeader{
    bool valid() const { return (data[0] & 0xff) == 0x69; }
    uint8_t adcLatch() const { return (data[0] >> 8) & 0xff; }
    uint64_t fpgaTimestamp() const { return ((data[1] >> 8) & 0xff)+((data[2] & 0xffff) << 8)+((data[3] & 0xffff) << 24); }
    uint16_t sampicTimestampAGray() const { return data[4] & 0xffff; }
    uint16_t sampicTimestampBGray() const { return data[5] & 0xffff; }
    uint16_t sampicTimeStampA() const { return grayDecode<uint16_t>(sampicTimestampAGray()); }
    uint16_t sampicTimeStampB() const { return grayDecode<uint16_t>(sampicTimestampBGray()); }
    uint16_t cellInfo() const { return data[6] & 0xffff; }
    std::array<uint16_t,7> data;
  };

  template<size_t N>
  struct SampicStream{
    SampicHeader header;
    std::array<uint16_t,N> data;
  };
  template<size_t N>
  struct ChannelStream{
    LpbusHeader header;
    SampicStream<N> sampic;
  };
  typedef ChannelStream<64> ChannelStream64;
  
  template<size_t N>
  struct Event{
    EventHeader header;
    std::vector<ChannelStream<N> > channels;
    uint16_t end_word;  
  };

  template<typename T> static T grayDecode(T gray) {
    T bin = gray;
    while (gray >>= 1)
      bin ^= gray;
    return bin;
  }

  uint32_t m_print_hdr = 0;
  uint32_t m_print_evt = 0;
  uint32_t m_num_sampic_mezz = 2;
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
    m_num_sampic_mezz = conf->Get("SAMPIC_NUM_MEZZANINES", 2);
  }
}

void SampicDataCollector::DoReset(){
  std::unique_lock<std::mutex> lk(m_mtx_map); // a bit of thread safety...
  m_print_hdr = 0;
  m_print_evt = 0;
  m_num_sampic_mezz = 2;
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
  auto ev_sync = eudaq::Event::MakeUnique("SampicTriggerEvent");

  //--- start filling the synchronised collection
  EventHeader evt_header;
  for (auto& conn_evque : m_conn_evque) {
    auto& ev_front = conn_evque.second.front();
    for (uint32_t i = 0; i < m_num_sampic_mezz; ++i) {
      auto data_block = ev_front->GetBlock(i);
      uint32_t offset = 0;
      std::memcpy(&evt_header, &data_block[offset], sizeof(evt_header));
      offset += sizeof(evt_header);
      if (!evt_header.valid())
        EUDAQ_THROW("Event header is not valid!");

      std::vector<ChannelStream64> channels;
      for (uint16_t i = 0; i < evt_header.activeChannels(); ++i) {
        ChannelStream64 channel;
        std::memcpy(&channel, &data_block[offset], sizeof(channel));
        offset += sizeof(channel);
        channels.emplace_back(channel);
      }

      if (ev_sync->GetTriggerN() == evt_header.triggerNumber()) {
        ev_sync->AddSubEvent(ev_front);
        ev_sync->SetEventN(evt_header.eventNumber());
        ev_sync->SetTriggerN(evt_header.triggerNumber());
      }
    }
  }
  //ev_sync->SetFlagPacket();
  //ev_sync->SetTriggerN(trigger_n);
  /*for (auto& conn_evque: m_conn_evque){
    auto& ev_front = conn_evque.second.front();
    if (ev_front->GetTriggerN() == trigger_n){
      ev_sync->AddSubEvent(ev_front);
      conn_evque.second.pop_front();
    }
  }

  if (!m_conn_inactive.empty()){
    std::set<eudaq::ConnectionSPC> conn_inactive_empty;
    for (auto& conn: m_conn_inactive){
      if (m_conn_evque.find(conn) != m_conn_evque.end() && m_conn_evque[conn].empty()) {
        m_conn_evque.erase(conn);
        conn_inactive_empty.insert(conn);
      }
    }
    for (auto& conn: conn_inactive_empty){
      m_conn_inactive.erase(conn);
    }
  }*/
  if (m_print_evt)
    ev_sync->Print(std::cout);

  WriteEvent(std::move(ev_sync));
}
