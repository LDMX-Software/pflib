#pragma once

#include <array>
#include <span>

#include "pflib/packing/Reader.h"
#include "pflib/packing/DAQLinkFrame.h"
#include "pflib/packing/TriggerLinkFrame.h"

namespace pflib::packing {

struct SingleROCEventPacket {
  std::array<DAQLinkFrame, 2> daq_links;
  std::array<TriggerLinkFrame, 4> trigger_links;
  void from(std::span<uint32_t> data);
  Reader& read(Reader& r);
  SingleROCEventPacket() = default;
};

}
