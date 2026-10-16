#pragma once
#ifndef PFLIB_PACKING_HCALECONTROCSTCMAPPING_H
#define PFLIB_PACKING_HCALECONTROCSTCMAPPING_H

#include <vector>

namespace pflib::packing {

/**
 * map ROC channels into ECON-T/STC
 *
 * The inverse mapping is not provided since many ROC channels
 * are summed into a single STC.
 *
 * This is for a single Hcal Backplane which hosts 2 ECON-Ts.
 * The 2 ECON-Ts on the backplane are indexed 0 and 1 in the
 * decoded packing::HcalECONTCaptureFrame but are indexed 1 and 2 in
 * the Target::econ accessor.
 */
class HcalECONTRocSTCMapping {
  /// hardware mapping copied from the Target
  std::vector<std::pair<int, std::vector<int>>> roc_trg_to_econt_erx_;

 public:
  HcalECONTRocSTCMapping(const std::vector<std::pair<int, std::vector<int>>>&
                             roc_trg_to_econt_erx);
  /**
   * which trigger channel the input ROC channel goes to
   *
   * The ROC channels are summed into Trigger Cells (TCs) in groups of 4
   * (for the Hcal) and channels whose index is equal to 8 modulo 9 are
   * ignored in the sum-by-4 configuration.
   * The TCs are then grouped together in groups of 4 to be sent along
   * the trigger channels to the ECON-T.
   *
   * @param[in] ch ROC channel [0,72)
   * @return the trigger channel ch would end up in, -1 if it will be ignored
   */
  static int toROCTrigChannel(int ch);
  /**
   * get the ECON-T/STC that the input ROC/channel would be summed into
   *
   * @param[in] i_roc ROC index
   * @param[in] ch channel within that ROC [0,72)
   * @return [i_econt, i_stc] where i_econt is the index of the ECON-T
   * in HcalECONTCaptureFrame and i_stc is the index of the STC in that ECON-T
   */
  std::pair<int, int> toECONTSTC(int i_roc, int ch) const;
};

}  // namespace pflib::packing

#endif
