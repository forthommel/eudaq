#ifndef srs_SrsBuffer_hh
#define srs_SrsBuffer_hh

#include "eudaq/Logger.hh"
#include <streambuf>

/// Output stream derivation to EUDAQ_INFO
class SrsBuffer : public std::ostream {
private:
  struct SrsLogger : public std::stringbuf {
    int sync() override {
      int ret = std::stringbuf::sync();
      EUDAQ_DEBUG(str());
      str("");
      return ret;
    }
  } buff_;

public:
  SrsBuffer() : buff_(), std::ostream(&buff_) {}
};

#endif
