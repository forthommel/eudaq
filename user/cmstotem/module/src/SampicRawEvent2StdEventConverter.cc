#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

#include "SampicDataFormat.hh"
#include "sampicdaq/sampic.h"

class SampicRawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("SampicRaw");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<SampicRawEvent2StdEventConverter>(SampicRawEvent2StdEventConverter::m_id_factory);
}

bool SampicRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  d2.reset(new eudaq::SampicEvent(*ev));
//  for (const auto& blk : ev->GetBlockNumList())
//    std::dynamic_pointer_cast<eudaq::SampicEvent>(d2)->ConvertBlock(ev->GetBlock(blk));
  /*size_t nblocks= ev->NumBlocks();
  auto block_n_list = ev->GetBlockNumList();
  for(auto &block_n: block_n_list){
    std::vector<uint8_t> block = ev->GetBlock(block_n);*/
    /*std::array<SampicEvent_t,512> events;
    int ret = sampic_reco_stream(block.data(), block.size(), events.data(), 1, 1, 1);
    EUDAQ_INFO("num events in block:"+std::to_string(ret));*/
    /*if(block.size() < 2)
      EUDAQ_THROW("Unknown data");
    uint8_t x_pixel = block[0];
    uint8_t y_pixel = block[1];
    std::vector<uint8_t> hit(block.begin()+2, block.end());
    if(hit.size() != x_pixel*y_pixel)
      EUDAQ_THROW("Unknown data");
    eudaq::StandardPlane plane(block_n, "sampic_plane", "sampic_plane");
    plane.SetSizeZS(hit.size(), 1, 0);
    for(size_t i = 0; i < y_pixel; ++i) {
      for(size_t n = 0; n < x_pixel; ++n){
	plane.PushPixel(n, i , hit[n+i*x_pixel]);
      }
    }
    d2->AddPlane(plane);*/
    
  //}
  return true;
}
