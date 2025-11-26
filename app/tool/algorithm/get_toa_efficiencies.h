#pragma once

#include <array>

#include "pflib/packing/MultiSampleECONDEventPacket.h"
#include "pflib/packing/SingleROCEventPacket.h"
#include "../pftool.h"

namespace pflib::algorithm {

/**
 * calculate the highest TOA_VREF value for each link, for which there is a
 * non-zero TOA efficiency
 */

// templated to match any event packet type
template <class EventPacket>
std::array<double, 72> get_toa_efficiencies(
    const std::vector<EventPacket>& data);

// std::array<double, 72> get_toa_efficiencies(
//     const std::vector<pflib::packing::SingleROCEventPacket>& data);

}  // namespace pflib::algorithm
