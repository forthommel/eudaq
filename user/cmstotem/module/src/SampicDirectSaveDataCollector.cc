#include "eudaq/DataCollector.hh"

namespace eudaq {
  class SampicDirectSaveDataCollector:public DataCollector{
    public:
      using DataCollector::DataCollector;
      void DoConfigure() override;
      void DoReceive(ConnectionSPC id, EventSP ev) override;
      static const uint32_t m_id_factory = cstr2hash("SampicDirectSaveDataCollector");

    private:
      bool m_ev_print;
  };

  namespace{
    auto dummy0 = Factory<DataCollector>::
      Register<SampicDirectSaveDataCollector, const std::string&, const std::string&>
      (SampicDirectSaveDataCollector::m_id_factory);
  }

  void SampicDirectSaveDataCollector::DoConfigure(){
    m_ev_print = false;
    auto conf = GetConfiguration();
    if (conf){
      conf->Print();
      m_ev_print = conf->Get("PRINT_RAW_EVENTS", 0);
    }
  }

  void SampicDirectSaveDataCollector::DoReceive(ConnectionSPC id, EventSP ev){
    if (m_ev_print)
      ev->Print(std::cout);
    WriteEvent(ev);
  }
}
