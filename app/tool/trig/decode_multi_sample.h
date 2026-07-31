#pragma once

#include <vector>
#include <span>

/**
 * apply SampleFrame::from for many samples of data
 *
 * @tparam SampleFrame the decoding packet that has a `from` method
 * @param[in] n_samples number of samples to decode
 * @param[in] data vector of words containing n_samples
 * @return vector of decoded SampleFrame packets
 */
template <class SampleFrame>
std::vector<SampleFrame> decode_multi_sample(int n_samples,
                                             std::vector<uint32_t>& data) {
  std::vector<SampleFrame> frames(n_samples);
  int offset{0};
  for (int i{0}; i < frames.size(); i++) {
    frames[i].from(std::span<uint32_t>(data.begin() + offset, data.end()));
    offset += frames[i].length();
  }
  return frames;
}

