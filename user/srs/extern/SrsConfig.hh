#ifndef SrsConfig_hh
#define SrsConfig_hh

#include "eudaq/StandardEvent.hh"
#include "srsdriver/ApvAppRegister.h"

namespace eudaq {
  class SrsConfig;
  using SrsConfigUP = Factory<SrsConfig>::UP;
  using SrsConfigSP = Factory<SrsConfig>::SP;
  using SrsConfigSPC = Factory<SrsConfig>::SPC;

  class DLLEXPORT SrsConfig : public StandardEvent {
  public:
    SrsConfig();
    SrsConfig(const Event&);

    void ConvertBlock(const std::vector<uint8_t>&);
    void Print(std::ostream&, size_t offset = 0) const override;

    static SrsConfigSP MakeShared();
    static const uint32_t m_id_factory = cstr2hash("SrsConfig");

  private:
    srs::ApvAppRegister apvapp_;
  };
}

#endif
