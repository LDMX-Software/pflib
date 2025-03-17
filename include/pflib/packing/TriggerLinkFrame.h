#pragma once

#include <span>
#include <cstdint>

namespace pflib::packing {

/**
 * Without a trigger link set up, the decoding cannot be fuctionally tested.
 *
 * This class is merely a skeleton of how unpacking could be done for these
 * trigger link frames.
 */
struct TriggerLinkFrame {
  int i_link;
  std::array<uint32_t, 4> data_words;

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
};

}

