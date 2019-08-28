#include "eudaq/DataCollector.hh"

namespace eudaq {
  class SampicDirectSaveDataCollector:public DataCollector{
    public:
      using DataCollector::DataCollector;
      void DoConfigure() override;
      void DoReceive(ConnectionSPC id, EventSP ev) override;
      static const uint32_t m_id_factory = cstr2hash("SampicDirectSaveDataCollector");

    private:
      uint32_t m_noprint;
  };

  namespace{
    auto dummy0 = Factory<DataCollector>::
      Register<SampicDirectSaveDataCollector, const std::string&, const std::string&>
      (SampicDirectSaveDataCollector::m_id_factory);
  }

  void SampicDirectSaveDataCollector::DoConfigure(){
    m_noprint = 0;
    auto conf = GetConfiguration();
    if(conf){
      conf->Print();
      m_noprint = conf->Get("DISABLE_PRINT", 0);
    }
  }

  void SampicDirectSaveDataCollector::DoReceive(ConnectionSPC id, EventSP ev){
    if(!m_noprint)
      ev->Print(std::cout);
    /*auto ev_out = Event::MakeUnique("SampicRaw");
    ev_out->AddSubEvent(ev);
    WriteEvent(std::move(ev_out));*/
    WriteEvent(ev);
  }
}
