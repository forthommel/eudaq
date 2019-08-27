#include "eudaq/TTreeEventConverter.hh"
#include "eudaq/RawEvent.hh"

#include "SampicDataFormat.hh"

class SampicRawEvent2TTreeEventConverter: public eudaq::TTreeEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::TTreeEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("SampicRaw");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::TTreeEventConverter>::
    Register<SampicRawEvent2TTreeEventConverter>(SampicRawEvent2TTreeEventConverter::m_id_factory);
}

bool SampicRawEvent2TTreeEventConverter::Converting(eudaq::EventSPC d1, eudaq::TTreeEventSP d2, eudaq::ConfigSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  uint32_t id = ev->GetExtendWord();
  std::cout << "ID " << id << std::endl;
  size_t nblocks= ev->NumBlocks();
  auto block_n_list = ev->GetBlockNumList();
  std:: cout << " blocks " << nblocks << std::endl;
  for(auto &block_n: block_n_list){
    std::vector<uint8_t> block = ev->GetBlock(block_n);
    if(block.size() < 2)
      EUDAQ_THROW("Unknown data");
    uint16_t* data = reinterpret_cast<uint16_t*>(block.data());
    uint8_t x_pixel = block[0];
    uint8_t y_pixel = block[1];
    std::vector<uint8_t> hit(block.begin()+2, block.end());
    std::vector<uint8_t> hitxv;
    if(hit.size() != x_pixel*y_pixel)
      EUDAQ_THROW("Unknown data");
    TString temp = "block";
    if (d2->GetListOfBranches()->FindObject(temp))  d2->SetBranchAddress(temp,&x_pixel);
    else
      d2->Branch(temp,&x_pixel,"x_pixel/b:y_pixel/b");
  }
  return true;
}
