#include "pflib/algorithm/get_toa_efficiencies.h"

#include "pflib/utility/efficiency.h"

namespace pflib::algorithm {

std::array<double, 72> get_toa_efficiencies(
    const std::vector<pflib::packing::SingleROCEventPacket>& data) {
  std::array<double, 72> efficiencies;
  /// reserve a vector of the appropriate size to avoid repeating allocation time for all 72 channels
  std::vector<int> toas(data.size());
  for (int ch{0}; ch < 72; ch++) {
    for (std::size_t i{0}; i < toas.size(); i++) {
      toas[i] = data[i].channel(ch).toa();
    }
    /// we assume that the data provided is not empty otherwise the efficiency calculation is meaningless
    efficiencies[ch] = pflib::utility::efficiency(toas);
  }
  return efficiencies;
}

}
