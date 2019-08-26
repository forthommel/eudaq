#include "eudaq/Monitor.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/StdEventConverter.hh"

#include "include/OnlineMon.hh"
//#include "include/OnlineMonWindow.hh"

#include "SampicEvent.hh"

#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>
#include <random>

#include "TGClient.h"

//class SampicMonitor : public eudaq::Monitor {
class SampicMonitor : public RootMonitor {
public:
  SampicMonitor(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override {}
  void DoStopRun() override {}
  void DoTerminate() override {}
  void DoReset() override {}
  void DoReceive(eudaq::EventSP ev) override;
  
  static const uint32_t m_id_factory = eudaq::cstr2hash("SampicMonitor");
  
private:
  bool m_en_print;
  bool m_en_std_converter;
  bool m_en_std_print;

  //OnlineMonWindow *m_onlinemon;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Monitor>::
    Register<SampicMonitor, const std::string&, const std::string&>(SampicMonitor::m_id_factory);
}

SampicMonitor::SampicMonitor(const std::string & name, const std::string & runcontrol)
  //:eudaq::Monitor(name, runcontrol),
  // m_onlinemon(nullptr){
  :RootMonitor(runcontrol, 0, 0, 800, 600, "", name){
  //m_onlinemon = new OnlineMonWindow(gClient->GetRoot(),800,600);
}

void SampicMonitor::DoInitialise(){
  auto ini = GetInitConfiguration();
  ini->Print(std::cout);
}

void SampicMonitor::DoConfigure(){
  auto conf = GetConfiguration();
  //conf->Print(std::cout);
  m_en_print = conf->Get("SAMPIC_ENABLE_PRINT", 1);
  m_en_std_converter = conf->Get("SAMPIC_ENABLE_STD_CONVERTER", 0);
  m_en_std_print = conf->Get("SAMPIC_ENABLE_STD_PRINT", 0);
}

void SampicMonitor::DoReceive(eudaq::EventSP ev){
  if(m_en_print)
    ev->Print(std::cout);
  if(m_en_std_converter){
    auto stdev = std::dynamic_pointer_cast<eudaq::SampicEvent>(ev);
    if(!stdev){
      stdev = eudaq::SampicEvent::MakeShared();
      eudaq::StdEventConverter::Convert(ev, stdev, nullptr); //no conf
    }
    if(m_en_std_print)
      stdev->Print(std::cout);
  }
}
