#include "get_toa_efficiencies.h"

#include "pflib/utility/efficiency.h"
#include <limits>

namespace pflib::algorithm {

// helper function
bool is_masked(int ch, const std::vector<int>& masked_channels) {
  return (count(masked_channels.begin(), masked_channels.end(), ch) > 0);
}

template <class EventPacket>
std::array<double, 72> get_toa_efficiencies(
    const std::vector<EventPacket>& data,
    const std::vector<int>& masked_channels) {
  std::array<double, 72> efficiencies;
  /// reserve a vector of the appropriate size to avoid repeating allocation
  /// time for all 72 channels
  // fill with NANs so that channels can be masked out
  efficiencies.fill(std::numeric_limits<double>::quiet_NaN());
  std::vector<int> toas(data.size());
  int i_ch = 0;  // 0–35
  int i_link = 0;

  for (int ch{0}; ch < 72; ch++) {
    if (is_masked(ch, masked_channels)) {
      continue;  // leave NaN
    }

    i_link = (ch / 36);
    i_ch = ch % 36;
    for (std::size_t i{0}; i < toas.size(); i++) {
      if constexpr (std::is_same_v<
                        EventPacket,
                        pflib::packing::MultiSampleECONDEventPacket>) {
        toas[i] = data[i].samples[data[i].i_soi].channel(i_link, i_ch).toa();
      } else if constexpr (std::is_same_v<
                               EventPacket,
                               pflib::packing::SingleROCEventPacket>) {
        toas[i] = data[i].channel(ch).toa();
      } else {
        PFEXCEPTION_RAISE("BadConf",
                          "Unable to do all_channels_to_csv for the "
                          "currently configured format.");
      }
    }
    /// we assume that the data provided is not empty otherwise the efficiency
    /// calculation is meaningless
    efficiencies[ch] = pflib::utility::efficiency(toas);
  }
  return efficiencies;
}

// -----------------------------------------------------------------------------
// Explicit template instantiations
// -----------------------------------------------------------------------------

// get toa efficiencies
template std::array<double, 72>
pflib::algorithm::get_toa_efficiencies<pflib::packing::SingleROCEventPacket>(
    const std::vector<pflib::packing::SingleROCEventPacket>& data,
    const std::vector<int>& masked_channels);
template std::array<double, 72> pflib::algorithm::get_toa_efficiencies<
    pflib::packing::MultiSampleECONDEventPacket>(
    const std::vector<pflib::packing::MultiSampleECONDEventPacket>& data,
    const std::vector<int>& masked_channels);

}  // namespace pflib::algorithm