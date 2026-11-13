#include "pflib/packing/EcalECONTCaptureFrame.h"

#include "pflib/packing/DecompressAEBM.h"
#include "pflib/packing/Mask.h"

namespace pflib::packing {
void EcalECONTSample::from(std::span<uint32_t> data) {
  if (data.size() != WORD_LEN) {
    pflib_log(error)
        << "Data received by EcalECONTSample is not length "
        << WORD_LEN << ", it is length " << data.size();
    return;
  }
  bx_ = ((data[0] >> 28) & mask<4>);
  sum_ = ((data[0] >> 20) & mask<8>);
  tc_ch_map_ = ((data[0] & mask<20>) << 28) | ((data[1] >> 3) & mask<28>);
  int bits_left = 3;
  int i_word = 1;
  for (int i{0}; i < N_TC; i++) {
    if (bits_left >= 6) {
      max_tc_[i] = ((data[i_word] >> (bits_left - 6)) & mask<6>);
      bits_left -= 6;
    } else {
      int high_part = ((data[i_word] & ((1 << bits_left) - 1)));
      int low_nbits = 6 - bits_left;
      bits_left = 32;
      i_word++;
      max_tc_[i] = ((high_part << low_nbits) | ((data[i_word] >> (bits_left - low_nbits)) & ((1 << low_nbits) - 1)));
    }
  }
}

int EcalECONTSample::bx() const { return bx_; }

int EcalECONTSample::sum() const {
  return decompressAEBM<5, 3>(encoded_sum());
}

int EcalECONTSample::encoded_sum() const {
  return sum_;
}

std::bitset<48> EcalECONTSample::ch_map() const {
  return tc_ch_map_;
}

int EcalECONTSample::tc(int i_tc) const {
  return max_tc_.at(i_tc);
}

}  // namespace pflib::packing
