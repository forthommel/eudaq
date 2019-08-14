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
extern t_DAQ DAQ_struct;
}
#include "QuickUSB.h"

QHANDLE hDevice;

//----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
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
  std::string m_def_card_path;
  std::string m_ini_card_path;
  bool m_flag_ts;
  //bool m_flag_tg;
  //uint32_t m_plane_id;
  FILE* m_file_lock;
  std::chrono::milliseconds m_ms_busy;
  bool m_exit_of_run;
  std::array<uint8_t,32000> m_buffer;
};
//----------DOC-MARK-----END*DEC-----DOC-MARK----------
//----------DOC-MARK-----BEG*REG-----DOC-MARK----------
namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<SampicProducer, const std::string&, const std::string&>(SampicProducer::m_id_factory);
}
//----------DOC-MARK-----END*REG-----DOC-MARK----------
//----------DOC-MARK-----BEG*CON-----DOC-MARK----------
SampicProducer::SampicProducer(const std::string & name, const std::string & runcontrol)
  :eudaq::Producer(name, runcontrol), m_file_lock(0), m_exit_of_run(false){
}
//----------DOC-MARK-----BEG*INI-----DOC-MARK----------
void SampicProducer::DoInitialise(){
  auto ini = GetInitConfiguration();
  std::string lock_path = ini->Get("SAMPIC_DEV_LOCK_PATH", "sampic.lock");
  m_file_lock = fopen(lock_path.c_str(), "a");
#ifndef _WIN32
  if (flock(fileno(m_file_lock), LOCK_EX|LOCK_NB)){ //fail
    EUDAQ_THROW("unable to lock the lockfile: "+lock_path );
  }
#endif
}

//----------DOC-MARK-----BEG*CONF-----DOC-MARK----------
void SampicProducer::DoConfigure(){
  auto conf = GetConfiguration();
  conf->Print(std::cout);
  //--- initialise the configuration parser
  menu_init();
  //--- find the QuickUSB interface
  if (!qusb_find_modules()) {
    //EUDAQ_THROW("No QuickUSB modules found!");
  }
  m_flag_ts = conf->Get("SAMPIC_ENABLE_TIMESTAMP", 0);
  //m_stop_second = conf->Get("SAMPIC_STOP_RUN_AFTER_N_SECONDS", 0);
  m_def_card_path = conf->Get("SAMPIC_DEF_CARD", "SampicDAQ_define.uic");
  m_ini_card_path = "@"+conf->Get("SAMPIC_INI_CARD", "SampicDAQ_init");
  //--- first add a few definitions
  define_file((char*)m_def_card_path.c_str());
  //--- then initialise the DAQ
  simulate_input((char*)m_ini_card_path.c_str());
  //m_plane_id = conf->Get("SAMPIC_PLANE_ID", 0);
}
//----------DOC-MARK-----BEG*RUN-----DOC-MARK----------
void SampicProducer::DoStartRun(){
  m_exit_of_run = false;
  if (strcmp(DAQ_struct.status,"Running")==0) {
    EUDAQ_THROW("Acquisition is already running!");
  }
  if (strcmp(DAQ_struct.status,"Ready")!=0) {
    EUDAQ_ERROR("Acquisition is not ready (status is \""
               +std::string(DAQ_struct.status)+"\"). "
               +"Aborting.");
    return;
  }

  //init daq params
  DAQ_struct.GlobEvtNum = 0;
  DAQ_struct.stop_req   = 0;

  if (DAQ_struct.GlobMaxEvt!=0)
    EUDAQ_INFO("A total of "+std::to_string(DAQ_struct.GlobMaxEvt)+" event(s) "
              +"will be (hopefully) collected.");
  else
    EUDAQ_INFO("Events will be collected until external stop request (or error...)");
}
//----------DOC-MARK-----BEG*STOP-----DOC-MARK----------
void SampicProducer::DoStopRun(){
  m_exit_of_run = true;
}
//----------DOC-MARK-----BEG*RST-----DOC-MARK----------
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
//----------DOC-MARK-----BEG*TER-----DOC-MARK----------
void SampicProducer::DoTerminate(){
  m_exit_of_run = true;
  if (m_file_lock){
    fclose(m_file_lock);
    m_file_lock = 0;
  }
}
//----------DOC-MARK-----BEG*LOOP-----DOC-MARK----------
void SampicProducer::RunLoop(){
  auto tp_start_run = std::chrono::steady_clock::now();
  while (!m_exit_of_run){
    auto ev = eudaq::Event::MakeUnique("SampicRaw");

    auto tp_trigger = std::chrono::steady_clock::now();
    auto tp_end_of_busy = tp_trigger + m_ms_busy;
    if (m_flag_ts){
      std::chrono::nanoseconds du_ts_beg_ns(tp_trigger - tp_start_run);
      ev->SetTimestamp(du_ts_beg_ns.count(), 0);
    }

    for (uint32_t feIndex=0; feIndex < 2; ++feIndex) {
      int nByte=0;
      if (SF2_RW_REG.DAQ_mode==1)
        nByte = acq_get_stream(feIndex, m_buffer.data());
      else
        nByte = acq_get_built_stream(feIndex, m_buffer.data(), 0);
      if (nByte > 0) {
        EUDAQ_INFO("got "+std::to_string(nByte)+" byte(s)");
        std::vector<uint8_t> data(m_buffer.begin(), m_buffer.begin()+nByte);
        ev->AddBlock(feIndex, data);
      }
    }
    uint32_t trigger_n = 0; //FIXME
    ev->SetTriggerN(trigger_n); //FIXME when to issue trigger

    SendEvent(std::move(ev));
  }
}
//----------DOC-MARK-----END*IMP-----DOC-MARK----------
