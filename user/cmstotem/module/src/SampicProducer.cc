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

extern "C" {
#include "sampicdaq/menus.h"
#include "sampicdaq/quickusb.h"
#include "sampicdaq/acq.h"
#include "sampicdaq/sf2.h"
}
#include "QuickUSB.h"

QHANDLE hDevice;

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

  bool m_flag_ts = false;
  FILE* m_file_lock = 0;
  std::chrono::milliseconds m_ms_busy;
  bool m_exit_of_run = false;
  std::array<uint8_t,m_buffer_size> m_buffer;
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
  std::string def_card_path = ini->Get("SAMPIC_DEF_CARD", "SampicDAQ_define.uic");
  std::string ini_card_path = ini->Get("SAMPIC_INI_CARD", "@SampicDAQ_init");
  
  //--- find the QuickUSB interface
  if (!qusb_find_modules())
    EUDAQ_THROW("No QuickUSB modules found!");

  //--- initialise the configuration parser
  menu_init();
  AllowInput = 1;

  //--- first add a few definitions
  define_file((char*)def_card_path.c_str());
  //--- then add the acquisition configuration part
  simulate_input((char*)ini_card_path.c_str());

  //--- launch the configuration loop
  Main_menu();

  EUDAQ_INFO("Successfully loaded the definitions file \""+def_card_path+"\"\n"
            +"and configured the module using \""+ini_card_path+"\".");

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
  DAQ_struct.GlobMaxEvt = conf->Get("SAMPIC_MAX_EVENTS", 0);

  //conf->Print(std::cout);
}

void SampicProducer::DoStartRun(){
  m_exit_of_run = false;
  std::string status(DAQ_struct.status);
  if (status == "Running")
    EUDAQ_THROW("Acquisition is already running!");
  if (status != "Ready")
    EUDAQ_THROW("Acquisition is not ready (status is \""+status+"\"). Aborting.");

  //init daq params
  DAQ_struct.GlobEvtNum = 0;
  DAQ_struct.stop_req = 0;

  if (DAQ_struct.GlobMaxEvt != 0)
    EUDAQ_INFO("A total of "+std::to_string(DAQ_struct.GlobMaxEvt)+" event(s) "
              +"will (hopefully) be collected.");
  else
    EUDAQ_INFO("Events will be collected until external stop request (or error...)");
}

void SampicProducer::DoStopRun(){
  m_exit_of_run = true;
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

  strcpy(DAQ_struct.status,"Ready");
  EUDAQ_INFO("DAQ status changed to \""+std::string(DAQ_struct.status)+"\".");
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
  while (!m_exit_of_run) {
    if (DAQ_struct.GlobMaxEvt > 0
     && DAQ_struct.GlobEvtNum == DAQ_struct.GlobMaxEvt) {
      EUDAQ_INFO("Maximum number of "+std::to_string(DAQ_struct.GlobMaxEvt)
                +" event(s) reached.");
      break;
    }
    if (std::string(DAQ_struct.status) == "Error")
      EUDAQ_THROW("Data taking aborted due to fatal error detection.");
    if (DAQ_struct.stop_req == 1) {
      EUDAQ_WARN("Data taking aborted due to external request.");
      break;
    }

    auto ev = eudaq::Event::MakeUnique("SampicRaw");

    auto tp_trigger = std::chrono::steady_clock::now();
    auto tp_end_of_busy = tp_trigger + m_ms_busy;
    if (m_flag_ts) {
      std::chrono::nanoseconds du_ts_beg_ns(tp_trigger - tp_start_run);
      ev->SetTimestamp(du_ts_beg_ns.count(), 0);
    }

    for (uint32_t feIndex=0; feIndex < 2; ++feIndex) {
      int num_bytes=0;
      if (SF2_RW_REG.DAQ_mode==1)
        num_bytes = acq_get_stream(feIndex, m_buffer.data());
      else
        num_bytes = acq_get_built_stream(feIndex, m_buffer.data(), 0);
      if (num_bytes < 0)
        EUDAQ_THROW("Acquisition failed with code "+std::to_string(num_bytes));
      if (num_bytes > m_buffer_size)
        EUDAQ_ERROR("Buffer overflown! ("+std::to_string(num_bytes)+" > max_bytes="
                   +std::to_string(m_buffer_size)+")");
      if (num_bytes > 0) {
        EUDAQ_INFO("got "+std::to_string(num_bytes)+" byte(s)");
        //--- reinterpret output as 16-bit words
        uint16_t* ptr = reinterpret_cast<uint16_t*>(m_buffer.data());
        ev->AddBlock(feIndex, std::vector<uint16_t>(ptr, ptr+num_bytes/2));
      }
    }
    //uint32_t trigger_n = 0; //FIXME
    //ev->SetTriggerN(trigger_n); //FIXME when to issue trigger

    SendEvent(std::move(ev));
  }
}

