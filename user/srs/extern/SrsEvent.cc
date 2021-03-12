#include "SrsEvent.hh"

#include <algorithm>

using namespace eudaq;

namespace {
  auto cstr_deser = Factory<Event>::Register<SrsEvent, Deserializer&>(SrsEvent::m_id_factory);
  auto cstr = Factory<Event>::Register<SrsEvent>(SrsEvent::m_id_factory);
}

SrsEventSP SrsEvent::MakeShared() {
  auto ev = Factory<Event>::MakeShared(m_id_factory);
  return std::dynamic_pointer_cast<SrsEvent>(ev);
}

SrsEvent::SrsEvent() {
  SetType(m_id_factory);
}

SrsEvent::SrsEvent(const Event& ev) {
  SetType(m_id_factory);
  for (auto& block_n : ev.GetBlockNumList())
    ConvertBlock(ev.GetBlock(block_n));
}

void SrsEvent::ConvertBlock(const std::vector<uint8_t>& block8) {

}

void SrsEvent::Print(std::ostream& os, size_t offset) const {
  os
    << std::string(offset, ' ') << "<SrsEvent>\n"
    << std::string(offset+2, ' ') << "<EventHeader>\n"
    << std::string(offset+2, ' ') << "</EventHeader>\n"
    << std::string(offset, ' ') << "</SrsEvent>\n";
}
