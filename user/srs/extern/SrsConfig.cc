#include "SrsConfig.hh"

using namespace eudaq;

namespace {
  auto cstr_deser = Factory<Event>::Register<SrsConfig, Deserializer&>(SrsConfig::m_id_factory);
  auto cstr = Factory<Event>::Register<SrsConfig>(SrsConfig::m_id_factory);
}

SrsConfigSP SrsConfig::MakeShared() {
  auto ev = Factory<Event>::MakeShared(m_id_factory);
  return std::dynamic_pointer_cast<SrsConfig>(ev);
}

SrsConfig::SrsConfig() {
  SetType(m_id_factory);
}

SrsConfig::SrsConfig(const Event& ev) {
  SetType(m_id_factory);
  for (auto& block_n : ev.GetBlockNumList())
    ConvertBlock(ev.GetBlock(block_n));
}

void SrsConfig::ConvertBlock(const std::vector<uint8_t>& block8) {
  srs::words_t block32; // convert 8-bit to 32-bit words
  for (size_t i = 0; i < block8.size()/4; i += 4)
    block32.emplace_back(
      block8[i] + (block8[i+1] << 8) + (block8[i+2] << 16) + (block8[i+3] << 24)
    );
  std::cout << "block: " << block32.size() << std::endl;
  //frmbuf_ = std::make_unique<srs::SrsFrame>(block32, eb_mode_);
}

void SrsConfig::Print(std::ostream& os, size_t offset) const {
  os
    << std::string(offset, ' ') << "<SrsConfig/>\n";
}
