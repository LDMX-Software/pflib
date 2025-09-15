#pragma once

#include "pflib/packing/Reader.h"
#include "pflib/packing/ECONDEventPacket.h"

namespace pflib::packing {

class MultiSampleECONDEventPacket {
  /// handle to logging source
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get("decoding")};
  int n_links_;
 public:
  int bx, ievent, contrib_id, run;
  std::vector<ECONDEventPacket> samples;
  MultiSampleECONDEventPacket(int n_links);
  void from(std::span<uint32_t> data);
  Reader& read(Reader& r);
};

}
