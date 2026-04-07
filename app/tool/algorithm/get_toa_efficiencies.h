#pragma once

#include <array>

#include "../pftool.h"
#include "pflib/packing/MultiSampleECONDEventPacket.h"
#include "pflib/packing/SingleROCEventPacket.h"

namespace pflib::algorithm {

/**
 * calculate the highest TOA_VREF value for each link, for which there is a
 * non-zero TOA efficiency
 */

// templated to match any event packet type
std::array<double, 72> get_toa_efficiencies(
    const std::vector<pflib::packing::MultiSampleECONDEventPacket>& data);

}  // namespace pflib::algorithm
