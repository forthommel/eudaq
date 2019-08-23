#include "eudaq/Producer.hh"
#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>
#include <random>
#include <array>
#ifndef _WIN32
#include <sys/file.h>
#endif

#include "sampicdaq/menus.h"
#include "sampicdaq/quickusb.h"
#include "sampicdaq/acq.h"
#include "sampicdaq/sampic.h"

class SampicProducer : public eudaq::Producer {
public:
  SampicProducer(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void RunLoop() override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("SampicProducer");

private:
  static const size_t m_buffer_size = 32000;
  void LoadConfiguration(std::string) const;

  std::string m_def_card_path;
  bool m_flag_ts = false;
  unsigned long m_max_events;
  unsigned long m_num_events = 0;
  unsigned long m_num_sampic_mezz = 2;
  FILE* m_file_lock = 0;
  std::chrono::milliseconds m_ms_busy;
  bool m_exit_of_run = false;
  std::array<uint8_t,m_buffer_size> m_buffer;
  QHANDLE hDevice;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<SampicProducer, const std::string&, const std::string&>(SampicProducer::m_id_factory);
}

SampicProducer::SampicProducer(const std::string & name, const std::string & runcontrol)
  :eudaq::Producer(name, runcontrol){
}

void SampicProducer::DoInitialise(){
  //--- parse the initialisation file
  auto ini = GetInitConfiguration();
  std::string lock_path = ini->Get("SAMPIC_DEV_LOCK_PATH", "sampic.lock");
  m_def_card_path = ini->Get("SAMPIC_DEF_CARD", "SampicDAQ_define.uic");
  std::string ini_card_path = ini->Get("SAMPIC_INI_CARD", "@SampicDAQ_init");
  //--- find the QuickUSB interface
  if (!qusb_find_modules())
    EUDAQ_THROW("No QuickUSB modules found!");

  LoadConfiguration(ini_card_path);
  EUDAQ_INFO("Successfully initialised the module using \""+ini_card_path+"\".");

  //--- introduce a lockfile
  m_file_lock = fopen(lock_path.c_str(), "a");
#ifndef _WIN32
  if (flock(fileno(m_file_lock), LOCK_EX|LOCK_NB)) //fail
    EUDAQ_THROW("Unable to lock the lockfile: \""+lock_path+"\"");
#endif
}

void SampicProducer::DoConfigure(){
  //--- parse the configuration file
  auto conf = GetConfiguration();
  m_flag_ts = conf->Get("SAMPIC_ENABLE_TIMESTAMP", 0);
  m_max_events = conf->Get("SAMPIC_MAX_EVENTS", 0);
  m_num_sampic_mezz = conf->Get("SAMPIC_NUM_MEZZANINES", 2);

  std::string run_card_path = conf->Get("SAMPIC_RUN_CARD", "@run_mode.uic");
  LoadConfiguration(run_card_path);
  EUDAQ_INFO("Successfully loaded the run card \""+run_card_path+"\"");
}

void SampicProducer::DoStartRun(){
  m_exit_of_run = false;

  //init daq params
  m_num_events = 0;
  DAQ_struct.stop_req = 0;

  if (m_max_events != 0)
    EUDAQ_INFO("A total of "+std::to_string(m_max_events)+" event(s) "
              +"will (hopefully) be collected.");
  else
    EUDAQ_INFO("Events will be collected until external stop request (or error...)");
}

void SampicProducer::DoStopRun(){
  m_exit_of_run = true;
  EUDAQ_INFO("Collected "+std::to_string(m_num_events)+" event(s)");
}

void SampicProducer::DoReset(){
  m_exit_of_run = true;
  if (m_file_lock){
#ifndef _WIN32
    flock(fileno(m_file_lock), LOCK_UN);
#endif
    fclose(m_file_lock);
    m_file_lock = 0;
  }
  m_ms_busy = std::chrono::milliseconds();
  m_exit_of_run = false;
}

void SampicProducer::DoTerminate(){
  m_exit_of_run = true;
  if (m_file_lock){
    fclose(m_file_lock);
    m_file_lock = 0;
  }
}

void SampicProducer::RunLoop(){
  auto tp_start_run = std::chrono::steady_clock::now();
  std::map<unsigned int,eudaq::EventUP> map_events;
  std::array<SampicEvent_t,512> events;

  while (!m_exit_of_run) {
    if (m_max_events > 0 && m_num_events == m_max_events) { //FIXME
      EUDAQ_INFO("Maximum number of "+std::to_string(m_num_events)+" event(s) reached.");
      return;
    }

    for (uint32_t feIndex=0; feIndex < m_num_sampic_mezz; ++feIndex) {
      int num_bytes = acq_get_built_stream(feIndex, m_buffer.data());
      //int num_bytes = acq_get_stream(feIndex, m_buffer.data());
      if (num_bytes < 0)
        EUDAQ_THROW("Acquisition failed with code "+std::to_string(num_bytes));
      if (num_bytes == 0)
        continue;
      if (num_bytes > m_buffer_size)
        EUDAQ_ERROR("Buffer overflown! ("+std::to_string(num_bytes)+" > max_bytes="
                   +std::to_string(m_buffer_size)+")");

      if (num_bytes > 0) {
        std::vector<uint8_t> data(m_buffer.begin(), m_buffer.begin()+num_bytes);
        int ret = sampic_reco_stream(data.data(), data.size(), events.data(), 1, 0, 0);
        if (ret > 0) {
          for (int i=0; i < ret; ++i) {
            // build a new event or retrieve an existing one
            auto& ev = map_events[events[i].header.triggerNumber];
            if (!ev) {
              ev = eudaq::Event::MakeUnique("SampicRaw");
              ev->SetTriggerN(events[i].header.triggerNumber);
              ev->SetEventN(events[i].header.eventNumber);
              if (m_flag_ts) {
                auto tp_trigger = std::chrono::steady_clock::now();
                std::chrono::nanoseconds du_ts_beg_ns(tp_trigger - tp_start_run);
                ev->SetTimestamp(du_ts_beg_ns.count(), 0);
              }
            }
            ev->AddBlock(feIndex, data);
          }
        }
      }
    }
    // send all packets in temporary map
    for (auto it = map_events.begin(); it != map_events.end(); /*no increment*/) {
      if (it->second->GetNumBlock() == m_num_sampic_mezz) {
        SendEvent(std::move(it->second));
        it = map_events.erase(it);
        if (m_num_events++ % 1000 == 0)
          EUDAQ_INFO("Number of events sent: "+std::to_string(m_num_events));
      }
      else
        ++it;
    }
  }
}

void SampicProducer::LoadConfiguration(std::string config) const{
  //--- initialise the configuration parser
  menu_init();
  AllowInput = 1;

  //--- first add a few definitions
  define_file((char*)m_def_card_path.c_str());

  //--- then add the configuration part
  simulate_input((char*)config.c_str());
  //simulate_input("ret");
  Main_menu();
}

