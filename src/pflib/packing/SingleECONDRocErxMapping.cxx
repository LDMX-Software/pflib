#include "pflib/packing/SingleECONDRocErxMapping.h"

#include "pflib/Exception.h"
#include <algorithm>

namespace pflib::packing {

SingleECONDRocErxMapping::SingleECONDRocErxMapping(
    const std::vector<std::pair<int, int>>& roc_half_to_erx,
    const std::vector<int>& active_rocs) {
  roc_half_to_erx_ = roc_half_to_erx;
  erx_to_roc_half_.resize(2 * roc_half_to_erx_.size());
  i_erx_to_erx_.clear();
  for (int i_roc{0}; i_roc < roc_half_to_erx_.size(); i_roc++) {
    auto [erx_half_0, erx_half_1] = roc_half_to_erx_.at(i_roc);
    erx_to_roc_half_[erx_half_0] = std::make_pair(i_roc, 0);
    erx_to_roc_half_[erx_half_1] = std::make_pair(i_roc, 1);
    if (std::find(active_rocs.begin(), active_rocs.end(), i_roc) !=
        active_rocs.end()) {
      i_erx_to_erx_.push_back(erx_half_0);
      i_erx_to_erx_.push_back(erx_half_1);
    }
  }
  std::sort(i_erx_to_erx_.begin(), i_erx_to_erx_.end());
  // maximum of 12 eRx for an ECON
  erx_to_i_erx_.resize(12);
  // mark inactive eRx with a negative index
  for (int& i_erx : erx_to_i_erx_) {
    i_erx = -1;
  }
  // invert map of indexed eRx
  for (std::size_t i_erx{0}; i_erx < i_erx_to_erx_.size(); i_erx++) {
    erx_to_i_erx_.at(i_erx_to_erx_[i_erx]) = i_erx;
  }
}

std::pair<int, int> SingleECONDRocErxMapping::toROCHalf(int i_erx) const {
  return erx_to_roc_half_.at(i_erx_to_erx_.at(i_erx));
}

std::pair<int, int> SingleECONDRocErxMapping::toROCChannel(int i_erx,
                                                           int channel) const {
  if (channel < 0 or channel >= 36) {
    PFEXCEPTION_RAISE(
        "BadChannel",
        "You gave an invalid channel with the eRx index (" +
            std::to_string(channel) +
            ") "
            "A channel specified with a eRx index is >= 0 and < 36.");
  }
  auto [iroc, half] = toROCHalf(i_erx);
  return std::make_pair(iroc, channel + half * 36);
}

int SingleECONDRocErxMapping::toErx(int i_roc, int half) const {
  if (half != 0 and half != 1) {
    PFEXCEPTION_RAISE(
        "BadHalf",
        "A ROC's half is either 0 (lower channels) or 1 (upper channels).");
  }
  const auto& [half_0_eRx, half_1_eRx] = roc_half_to_erx_.at(i_roc);
  return erx_to_i_erx_.at((half == 0) ? half_0_eRx : half_1_eRx);
}

std::pair<int, int> SingleECONDRocErxMapping::toErxChannel(int i_roc,
                                                           int channel) const {
  if (channel < 0 or channel >= 72) {
    PFEXCEPTION_RAISE(
        "BadChannel",
        "You gave an invalid channel with the ROC index (" +
            std::to_string(channel) +
            ") "
            "A channel specified with a ROC index is >= 0 and < 72.");
  }
  int half = channel / 36;
  int channel_in_erx = channel % 36;
  return std::make_pair(toErx(i_roc, half), channel_in_erx);
}

}  // namespace pflib::packing
