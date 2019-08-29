#include "SampicEvent.hh"

#include <algorithm>

using namespace eudaq;

namespace{
  auto dummy0 = Factory<Event>::Register<SampicEvent, Deserializer&>(SampicEvent::m_id_factory);
  auto dummy1 = Factory<Event>::Register<SampicEvent>(SampicEvent::m_id_factory);
}

SampicEventSP SampicEvent::MakeShared(){
  auto ev = Factory<Event>::MakeShared(m_id_factory);
  return std::dynamic_pointer_cast<SampicEvent>(ev);
}

SampicEvent::SampicEvent()
{
  SetType(m_id_factory);
}

SampicEvent::SampicEvent(const Event& ev)
{
  SetType(m_id_factory);
  for (auto& block_n: ev.GetBlockNumList())
    ConvertBlock(ev.GetBlock(block_n));
}

SampicEvent::SampicEvent(const SampicEvent& ev)
  :m_header(ev.m_header), m_ch_stream(ev.m_ch_stream), m_trailer(ev.m_trailer){
}

SampicEvent::SampicEvent(Deserializer& ds) :
  StandardEvent(ds)
{
  std::cout << __PRETTY_FUNCTION__ << " not yet implemented!" << std::endl;
  //FIXME implement ds.read(...) methods
}

void SampicEvent::Serialize(Serializer& ser) const
{
  StandardEvent::Serialize(ser);
  ser.write(*m_header.data());
  for (const auto& ch : m_ch_stream) {
    ser.write(*ch.header.data());
    ser.write(*ch.sampic.header.data());
    ser.write(*ch.sampic.samples.data());
  }
  ser.write(*m_trailer.data());
}

void SampicEvent::ConvertBlock(const std::vector<uint8_t>& block8)
{
  // first convert byte stream into short stream
  std::vector<uint16_t> block;
  for (size_t i = 0; i < block8.size()/2; ++i)
    block.emplace_back(
       (block8.at(2*i)         & 0x00ff) |
      ((block8.at(2*i+1) << 8) & 0xff00));
  // then cast everything in its right place
  auto it = block.begin();
  while (it != block.end()) {
    std::copy_n(it, sizeof(sampic::EventHeader)/sizeof(uint16_t), m_header.begin());
    it += sizeof(sampic::EventHeader)/sizeof(uint16_t);

    // count the number of active channels in event
    size_t channel_in_evt = 0;
    uint16_t ch_map = m_header.activeChannels();
    while (ch_map) {
      channel_in_evt += ch_map & 0x1;
      ch_map >>= 1;
    }

    // unpack each channel
    for (size_t i = 0; i < channel_in_evt; ++i) {
      sampic::ChannelStream<64> stream;

      std::copy_n(it, sizeof(sampic::LpbusHeader)/sizeof(uint16_t), stream.header.begin());
      it += sizeof(sampic::LpbusHeader)/sizeof(uint16_t);
      std::copy_n(it, sizeof(sampic::SampicHeader)/sizeof(uint16_t), stream.sampic.header.begin());
      it += sizeof(sampic::SampicHeader)/sizeof(uint16_t);

      // unpack each sample
      for (size_t j = 0; j < stream.sampic.samples.size(); ++j) {
        stream.sampic.samples[j] = *it;
        it++;
      }
      m_ch_stream.push_back(stream);
    }
    std::copy_n(it, sizeof(sampic::EventTrailer)/sizeof(uint16_t), m_trailer.begin());
    if (!m_trailer.valid())
      throw std::runtime_error("Invalid trailer ("+std::to_string(*it)+") retrieved!");
    it += sizeof(sampic::EventTrailer)/sizeof(uint16_t);
  }
}

void SampicEvent::Print(std::ostream& os, size_t offset) const
{
  os << std::string(offset, ' ') << "<SampicEvent>\n";
  os << std::string(offset+2, ' ') << "<EventHeader valid=\"" << m_header.valid() << "\">\n";
  os << std::string(offset+4, ' ') << "<BoardId>" << (short)m_header.boardId() << "</BoardId>\n";
  os << std::string(offset+4, ' ') << "<SampicId>" << (short)m_header.sampicId() << "</SampicId>\n";
  os << std::string(offset+4, ' ') << "<TriggerNumber>" << m_header.triggerNumber() << "</TriggerNumber>\n";
  os << std::string(offset+4, ' ') << "<EventNumber>" << m_header.eventNumber() << "</EventNumber>\n";
  os << std::string(offset+4, ' ') << "<SamplesNumber>" << (short)m_header.sampleNumber() << "</SamplesNumber>\n";
  os << std::string(offset+4, ' ') << "<FWVersion>" << (short)m_header.fwVersion() << "</FWVersion>\n";
  os << std::string(offset+2, ' ') << "</EventHeader>\n";
  os << std::string(offset+2, ' ') << "<Channels>\n";
  for (const auto& st : m_ch_stream) {
    os << std::string(offset+4, ' ') << "<Channel id=\"" << (short)st.header.channelIndex() << "\">\n";
    os << std::string(offset+6, ' ') << "<FPGATimestamp>" << st.sampic.header.fpgaTimestamp() << "</FPGATimestamp>\n";
    os << std::string(offset+6, ' ') << "<SampicTimestampA>" << st.sampic.header.sampicTimeStampA() << "</SampicTimestampA>\n";
    os << std::string(offset+6, ' ') << "<SampicTimestampB>" << st.sampic.header.sampicTimeStampB() << "</SampicTimestampB>\n";
    os << std::string(offset+4, ' ') << "</Channel>\n";
  }
  os << std::string(offset+2, ' ') << "</Channels>\n";
  StandardEvent::Print(os,offset+2);
  os << std::string(offset, ' ') << "</SampicEvent>\n";
}

