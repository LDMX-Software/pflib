#include "pflib/packing/HcalECONTRocSTCMapping.h"

namespace pflib::packing {

HcalECONTRocSTCMapping::HcalECONTRocSTCMapping(
    const std::vector<std::pair<int, std::vector<int>>>& roc_trg_to_econt_erx)
    : roc_trg_to_econt_erx_{roc_trg_to_econt_erx} {}

int HcalECONTRocSTCMapping::toROCTrigChannel(int ch) {
  // some channels are ignored in the sum-by-4 configuration and not
  // included in any TC
  if (ch % 9 == 8) return -1;
  // we can calculate the TC index the channel ends up in with an ugly formula
  // where the division is intentionally integer division
  // (multiple channels ends up in the same TC)
  int roc_tc = 2 * (ch / 9) + ((ch % 9) / 4);
  // four TCs are included in one trig channel
  // so again integer division intentionally grouping many TCs
  return roc_tc / 4;
}

std::pair<int, int> HcalECONTRocSTCMapping::toECONTSTC(int i_roc,
                                                       int ch) const {
  auto [econt_id, erxs] = roc_trg_to_econt_erx_.at(i_roc);
  int roc_trg_channel = toROCTrigChannel(ch);
  if (roc_trg_channel < 0) {
    // the input ROC channel is not included in any STC
    return std::make_pair(econt_id - 1, -1);
  }
  /**
   * Each ROC trig channel gets mapped directly onto an ECON-T eRx
   * and each ECON-T eRx, since the ECON-T is assumed to be summing
   * 4 TCs together into an STC and the trig channel contains 4 TCs,
   * is identical to the STC index.
   *
   * The ECON-T IDs kept in the mapping are for the Target::econ
   * accessor, the HcalECONTCaptureFrame does not have the ECON-D
   * so the indices are are lowered by one.
   */
  return std::make_pair(econt_id - 1, erxs.at(roc_trg_channel));
}

}  // namespace pflib::packing
