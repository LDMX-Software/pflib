#pragma once

#include <array>

#include "pflib/packing/SingleROCEventPacket.h"

namespace pflib::algorithm {

/**
 * calculate the highest TOA_VREF value for each link, for which there is a
 * non-zero TOA efficiency
 */
std::array<double, 72> get_toa_efficiencies(
    const std::vector<pflib::packing::SingleROCEventPacket>& data);

}  // namespace pflib::algorithm
