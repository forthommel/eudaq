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
  frmbuf_.emplace_back(
      std::move(std::make_unique<srs::SrsFrame>(block32, eb_mode_)));
}

srs::SrsFrame *SrsEvent::Data(size_t i) const {
  if (i >= NumFrames())
    EUDAQ_THROW("SrsEvent frame counter overflow: requested frame #" +
                std::to_string(i + 1) + ", while only " +
                std::to_string(NumFrames()) + " is/are available.");
  return frmbuf_.at(i).get();
}

void SrsEvent::Print(std::ostream &os, size_t offset) const {
  os << std::string(offset, ' ') << "<SrsEvent>\n";
  for (size_t i = 0; i < NumFrames(); ++i) {
    const auto &frame = frmbuf_.at(i);
    os << std::string(offset + 2, ' ') << "<SrsFrame>\n"
       << std::string(offset + 4, ' ') << "<FrameCounter>\n"
       << std::string(offset + 6, ' ') << "<!--\n";
    frame->frameCounter().print(os);
    os << std::string(offset + 6, ' ') << "-->\n"
       << std::string(offset + 4, ' ') << "</FrameCounter>\n"
       << std::string(offset + 4, ' ') << "<DataSource>"
       << frame->dataSourceStr() << "</DataSource>\n"
       << std::string(offset + 4, ' ') << "<DaqChannel>"
       << (uint16_t)frame->daqChannel() << "</DaqChannel>\n"
       << std::string(offset + 4, ' ') << "<ApvIndex>"
       << (uint16_t)frame->apvIndex() << "</ApvIndex>\n"
       << std::string(offset + 4, ' ') << "<HeaderInfo>\n"
       << std::string(offset + 6, ' ') << "<AdcSize>"
       << frame->headerInfo().adcSize() << "</AdcSize>\n"
       << std::string(offset + 6, ' ') << "<FecId>0x" << std::hex
       << frame->headerInfo().fecId() << std::dec << "</FecId>\n"
       << std::string(offset + 4, ' ') << "</HeaderInfo>\n"
       << std::string(offset + 2, ' ') << "</SrsFrame>\n";
  }
  os << std::string(offset, ' ') << "</SrsEvent>\n";
}
