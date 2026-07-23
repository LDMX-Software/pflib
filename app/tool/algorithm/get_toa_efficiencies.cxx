#include "get_toa_efficiencies.h"

#include "pflib/logging/Logging.h"
#include "pflib/utility/efficiency.h"
namespace pflib::algorithm {

std::array<double, 72> get_toa_efficiencies(
    int i_roc, const pflib::packing::SingleECONDRocErxMapping& mapping,
    const std::vector<pflib::packing::MultiSampleECONDEventPacket>& data) {
  static auto the_log_{::pflib::logging::get("get_toa_efficiencies")};

  std::array<double, 72> efficiencies;

  /// reserve a vector of the appropriate size to avoid repeating allocation
  /// time for all 72 channels

  if (data.empty()) {
    pflib_log(warn) << "[DEBUG TOA] data packet vector is EMPTY for ROC "
                    << i_roc;
    efficiencies.fill(0.0);
    return efficiencies;
  }

  std::vector<int> toas(data.size());
  for (int ch{0}; ch < 72; ch++) {
    // TODO: 348

    auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, ch);

    if (i_erx < 0 || i_ch < 0) {
      pflib_log(error)
          << "[DEBUG MAP] Sanity Failure: Negative mapping values detected "
          << "ROC " << i_roc << " Ch " << ch
          << " resolved to eRx = " << (int)i_erx << ", eCh = " << (int)i_ch;
    }

    for (std::size_t i{0}; i < toas.size(); i++) {
      toas[i] = data[i].soi().channel(i_erx, i_ch).toa();
    }

    /// we assume that the data provided is not empty otherwise the efficiency
    /// calculation is meaningless

    if (ch == 0 || ch == 36) {
      pflib_log(trace) << "[DEBUG TOA] ROC " << i_roc << " Ch " << ch
                       << " (eRx: " << (int)i_erx << ", eCh: " << (int)i_ch
                       << ")"
                       << " first 3 raw TOAs: [" << toas[0] << ", "
                       << (toas.size() > 1 ? std::to_string(toas[1]) : "N/A")
                       << ", "
                       << (toas.size() > 2 ? std::to_string(toas[2]) : "N/A")
                       << "]";
    }

    efficiencies[ch] = pflib::utility::efficiency(toas);

    if (efficiencies[ch] > 0.0) {
      pflib_log(debug) << "[DEBUG TOA] Found non-zero efficiency on ROC "
                       << i_roc << " Ch " << ch << " = " << efficiencies[ch];
    }
  }
  return efficiencies;
}
}  // namespace pflib::algorithm
