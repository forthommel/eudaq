#ifndef SampicMonitor_hh
#define SampicMonitor_hh

#include "eudaq/StandardEvent.hh"
#include "eudaq/StdEventConverter.hh"

#include "include/OnlineMon.hh"

//#include <iostream>
//#include <fstream>
//#include <ratio>
//#include <chrono>
//#include <thread>
//#include <random>

//class SampicMonitor : public eudaq::Monitor {
class SampicMonitor : public RootMonitor {
public:
  SampicMonitor(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  /*void DoStartRun() override {}
  void DoStopRun() override {}
  void DoTerminate() override {}
  void DoReset() override {}*/
  void DoReceive(eudaq::EventSP ev) override;
  
  static const uint32_t m_id_factory = eudaq::cstr2hash("SampicMonitor");
  
private:
  bool m_en_print;
  bool m_en_std_converter;
  bool m_en_std_print;

  //OnlineMonWindow *m_onlinemon;
};

#ifdef __CINT__
#pragma link C++ class SampicMonitor - ;
#endif

#endif
