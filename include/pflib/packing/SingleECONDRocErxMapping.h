#pragma once
#ifndef PFLIB_PACKING_ROCERXMAPPING_H
#define PFLIB_PACKING_ROCERXMAPPING_H

#include <vector>

namespace pflib::packing {

/**
 * Mapping for link/channel IDs for single ECON-D collection
 *
 * @note As the name implies, this logic only works for single-ECON-D
 * setups where all the ROCs are going into a single ECON-D.
 *
 * Invert the ROC -> eRx pair mapping including which ROCs are active
 *
 * As far as I (Tom) know, the data is packed into the ECON-D packet
 * in eRx order which, depending on the wiring of the board connecting
 * the ROCs to the ECON-D, can lead to the "first" link not being eRx 0.
 * This means we need to do two "mappings" to get back the ROC-half a
 * specific link from the decoded data applies to.
 * 1. Map from link index (i_erx) to the eRx of the ECON-D
 *    (e.g. 0 -> 2 for HGCROC0 on the HcalBackplane)
 * 2. Map from eRx of the ECON-D to the ROC-half
 *
 * For example, if HGCROC0 is active on an HcalBackplane, its pair
 * of output links are mapped to eRx (3, 2) which are then packed
 * into the ECONDEventPacket in eRx order and unpacked with i_erx (0, 1).
 * i_erx 0 was eRx 2 which was the upper half (half 1) of HGCROC0.
 *
 * Since we are dealing with re-mapping of indices and integers, I don't
 * use std::map - I just use std::vector (but I'm still thinking of them
 * as maps where the "keys" are the vector indices).
 */
class SingleECONDRocErxMapping {
  /// mapping from ROC-half to eRx input to ECON-D
  std::vector<std::pair<int, int>> roc_half_to_erx_;
  /**
   * map from i_erx index in decoding to eRx ID input to ECON
   *
   * This depends on which ROCs are active.
   */
  std::vector<int> i_erx_to_erx_;

  /**
   * Map from eRx ID input to ECON to i_erx index in decoding
   *
   * This depends on which ROCs are active.
   */
  std::vector<int> erx_to_i_erx_;

  /**
   * map from eRx ID input to ECON to ROC-half
   *
   * This does not depend on which ROCs are active, but it
   * does depend on how the hardware is wired.
   * It is just an inversion of roc_half_to_erx_ that is
   * given to us in the constructor.
   */
  std::vector<std::pair<int, int>> erx_to_roc_half_;

 public:
  /**
   * Construct the mapping
   *
   * We need to know two pieces of information:
   * 1. Which ROC indices are active (i.e. would be included in the data packet)
   * 2. How the hardware connects the ROCs to the eRx input to the ECON-D
   *
   * @param[in] roc_half_to_erx mapping of ROC halfs to the eRx input to ECON-D
   * @param[in] active_rocs the ROC indices that are active
   *
   * The main work of this constructor is inverting the input mapping and
   * constructing the i_erx<->eRx mappings from the list of active_rocs. This
   * "inversion" is done once to make the later conversion of these indices
   * faster.
   */
  SingleECONDRocErxMapping(
      const std::vector<std::pair<int, int>>& roc_half_to_erx,
      const std::vector<int>& active_rocs);

  /**
   * Get which ROC-half the input i_erx corresponds to
   *
   * @param[in] i_erx index for the eRx link that the ECON-D output
   * @return [i_roc, half] where i_roc is the ROC index and half is
   * 0 (lower) or 1 (upper)
   */
  std::pair<int, int> toROCHalf(int i_erx) const;

  /**
   * Convert the input (i_erx, channel) index to the (i_roc, channel) index
   * using the RocErxMapping defined for the Target and which ROCs are active.
   *
   * @param[in] i_erx index for the eRx link that the ECON-D as output after
   * decoding
   * @param[in] channel channel index within that eRx (0-36)
   * @return [i_roc, channel] where i_roc is the ROC index (retrieve with
   * roc(i_roc)) and channel is the channel within that ROC (0-72)
   */
  std::pair<int, int> toROCChannel(int i_erx, int channel) const;

  /**
   * Get the i_erx the input ROC-half corresponds to
   *
   * @param[in] i_roc ROC index
   * @param[in] half 0 (lower) or 1 (upper) for which half of the ROC
   * @return i_erx index fo rthe eRx link that the ECON-D output
   */
  int toErx(int i_roc, int half) const;

  /**
   * Convert the input (i_roc, channel) index to the (i_erx, channel) index
   *
   * @param[in] i_roc index for an active ROC (no checking is done)
   * @param[in] channel channel index within that ROC (0-72)
   * @return [i_erx, channel] where i_erx is the index of the eRx input into
   * the ECON-D as output after decoding and channel is the channel within
   * that eRx (0-36)
   */
  std::pair<int, int> toErxChannel(int i_roc, int channel) const;
};

}  // namespace pflib::packing

#endif
