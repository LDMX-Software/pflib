#pragma once

#include <cstdint>
#include <span>

#include "pflib/Logging.h"

namespace pflib::packing {

/**
 * Convert a compressed trigger sum into its linearized equivalent
 *
 * The chip lossy compresses its trigger sums into seven bits
 * and this function decompresses the sum into our best estimate
 * of what the on-chip trigger sum was.
 *
 * @note This function undoes the wacky compression algorithm, but
 * the scale of the linearized sum is not quite correct.
 * If SelTC4 = 1 on the chip (sums of 4 channels), then this result
 * needs to be multiplied by 2 (or left-bit-shift by one).
 * If SelTC4 = 0 on the chip (sums of 9 channels), then this result
 * needs to be multiplied by 8 (or left-bit-shift by three)
 *
 * If SelTC4 stores the setting on the chip when the data was collected,
 * ```cpp
 * uint32_t ts = compressed_to_linearized(cs) << (SelTC4 ? 1 : 3);
 * ```
 *
 * @param[in] cs compressed sum to unpack
 * @return unpacked linearized sum
 */
uint32_t compressed_to_linearized(uint8_t cs);

/**
 * Without a trigger link set up, the decoding cannot be fuctionally tested.
 *
 * This class is merely a skeleton of how unpacking could be done for these
 * trigger link frames.
 */
struct TriggerLinkFrame {
  /// index of data words containg sample of interest
  static const int i_soi_ = 1;

  /// trigger link identification number
  int i_link;

  /**
   * Trigger words including one pre-sample and 2 following samples
   *
   * A trigger word contains 4 7-bit trigger sums and a 4-bit leading
   * header to help check for corruption and link alignment.
   */
  std::array<uint32_t, 4> data_words;

  /**
   * Checks on data format
   *
   * Index | Description
   * ------|-------------
   *  0    | leading four-bits of sw header is wrong
   *  1    | leading four-bits of 0'th data word is wrong
   *  2    | leading four-bits of 1st data word is wrong
   *  3    | leading four-bits of 2nd data word is wrong
   *  4    | leading four-bits of 3rd data word is wrong
   */
  std::array<bool, 5> corruption;

  /**
   * Get a specific compressed trigger sum from the data words
   *
   * @param[in] i_sum sum index 0, 1, 2, or 3
   * @param[in] i_bx bunch index relative to i_soi_ -1, 0, 1, or 2
   * @return compressed trigger sum as determined by the chip
   */
  uint8_t compressed_sum(int i_sum, int i_bx = 0) const;

  /**
   * Get a specific linearized trigger sum from the data words
   *
   * @see compressed_sum for how accessing a specific sum within
   * a trigger word is implemented.
   * @see compressed_to_linearized for how this compressed seven-bit
   * sum is unpacked into the linearized form.
   *
   * @param[in] i_sum sum index 0, 1, 2, or 3
   * @param[in] i_bx bunch index relative to i_soi_ -1, 0, 1, or 2
   * @return linearized trigger sum as determined by the chip
   */
  uint32_t linearized_sum(int i_sum, int i_bx = 0) const;

  /**
   * Parse into this link frame from a std::span over 32-bit words
   */
  void from(std::span<uint32_t> data);

  /**
   * Construct from a std::span over 32-bit words
   *
   * @note
   * A std::span can be transparently constructed from a std::vector
   * if that is what is available, but it can also be used to simply
   * view a subslice of a larger std::vector.
   */
  TriggerLinkFrame(std::span<uint32_t> data);
  TriggerLinkFrame() = default;
  mutable logging::logger the_log_{logging::get("decoding")};
};

}  // namespace pflib::packing
