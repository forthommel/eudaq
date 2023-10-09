#include "SrsEvent.hh"

#include <algorithm>

using namespace eudaq;

namespace {
  auto cstr_deser = Factory<Event>::Register<SrsEvent, Deserializer &>(
      SrsEvent::m_id_factory);
  auto cstr = Factory<Event>::Register<SrsEvent>(SrsEvent::m_id_factory);
} // namespace

SrsEventSP SrsEvent::MakeShared() {
  auto ev = Factory<Event>::MakeShared(m_id_factory);
  return std::dynamic_pointer_cast<SrsEvent>(ev);
}

SrsEvent::SrsEvent(const srs::ApvAppRegister::EventBuilderMode &ebmode)
    : eb_mode_(ebmode) {
  SetType(m_id_factory);
}

SrsEvent::SrsEvent(const Event &ev,
                   const srs::ApvAppRegister::EventBuilderMode &ebmode)
    : eb_mode_(ebmode) {
  SetType(m_id_factory);
  for (const auto &block_n : ev.GetBlockNumList())
    ConvertBlock(ev.GetBlock(block_n));
}

void SrsEvent::ConvertBlock(const std::vector<uint8_t> &block8) {
  srs::words_t block32; // convert 8-bit to 32-bit words
  for (size_t i = 0; i < block8.size(); i += 4)
    block32.emplace_back(((block8[i] & 0xff) + ((block8[i + 1] & 0xff) << 8) +
                          ((block8[i + 2] & 0xff) << 16) +
                          ((block8[i + 3] & 0xff) << 24)) &
                         0xffffffff);
  frmbuf_ = std::make_unique<srs::SrsFrame>(block32, eb_mode_);
}

void SrsEvent::Print(std::ostream &os, size_t offset) const {
  if (!frmbuf_)
    EUDAQ_THROW("SrsEvent object was not properly converted before printing.");

  os << std::string(offset, ' ') << "<SrsEvent>\n"
     << std::string(offset + 2, ' ') << "<FrameCounter>\n"
     << std::string(offset + 4, ' ') << "<!--\n";
  frmbuf_->frameCounter().print(os);
  os << std::string(offset + 4, ' ') << "-->\n"
     << std::string(offset + 2, ' ') << "</FrameCounter>\n"
     << std::string(offset + 2, ' ') << "<DataSource>"
     << frmbuf_->dataSourceStr() << "</DataSource>\n"
     << std::string(offset + 2, ' ') << "<DaqChannel>"
     << (uint16_t)frmbuf_->daqChannel() << "</DaqChannel>\n"
     << std::string(offset + 2, ' ') << "<ApvIndex>"
     << (uint16_t)frmbuf_->apvIndex() << "</ApvIndex>\n"
     << std::string(offset + 2, ' ') << "<HeaderInfo>\n"
     << std::string(offset + 4, ' ') << "<AdcSize>"
     << frmbuf_->headerInfo().adcSize() << "</AdcSize>\n"
     << std::string(offset + 4, ' ') << "<FecId>0x" << std::hex
     << frmbuf_->headerInfo().fecId() << std::dec << "</FecId>\n"
     << std::string(offset + 2, ' ') << "</HeaderInfo>\n"
     << std::string(offset, ' ') << "</SrsEvent>\n";
}
