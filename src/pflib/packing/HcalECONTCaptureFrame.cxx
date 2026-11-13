#include "pflib/packing/HcalECONTCaptureFrame.h"

#include "pflib/packing/DecompressAEBM.h"
#include "pflib/packing/Mask.h"

namespace pflib::packing {

void HcalBackplaneECONTPairSample::from(std::span<uint32_t> data) {
  if (data.size() != WORD_LEN) {
    pflib_log(error)
        << "Data received by HcalBackplaneECONTPairSample is not length "
        << WORD_LEN << ", it is length " << data.size();
    return;
  }
  for (int i_econt{0}; i_econt < 2; i_econt++) {
    bx_[i_econt] = ((data[3*i_econt] >> 28) & mask<4>);
    for (int i{0}; i < N_STC; i++) {
      // max is all within the first 32b word even if there
      // are more eTx
      max_tc_[8*i_econt+i] = ((data[3*i_econt] >> (26 - 2 * i)) & mask<2>);
    }
  
    // just hardcoding 3 eTx per ECON-T
    stc_sums_[8*i_econt+0] = ((data[3*i_econt] >> 3) & mask<9>);
    stc_sums_[8*i_econt+1] =
        (((data[3*i_econt] & mask<3>) << 6) | ((data[3*i_econt+1] >> (32 - 6)) & mask<6>));
    stc_sums_[8*i_econt+2] = ((data[3*i_econt+1] >> 17) & mask<9>);
    stc_sums_[8*i_econt+3] = ((data[3*i_econt+1] >> 8) & mask<9>);
    stc_sums_[8*i_econt+4] = (((data[3*i_econt+1] & mask<8>) << 1) | ((data[3*i_econt+2] >> 31) & mask<1>));
    stc_sums_[8*i_econt+5] = ((data[3*i_econt+2] >> 22) & mask<9>);
    stc_sums_[8*i_econt+6] = ((data[3*i_econt+2] >> 13) & mask<9>);
    stc_sums_[8*i_econt+7] = ((data[3*i_econt+2] >> 4) & mask<9>);
  }
}

int HcalBackplaneECONTPairSample::bx(int i_econt) const { return bx_[i_econt]; }

int HcalBackplaneECONTPairSample::stc_sum(int i_stc) const {
  return decompressAEBM<5, 4>(encoded_stc_sum(i_stc));
}

int HcalBackplaneECONTPairSample::encoded_stc_sum(int i_stc) const {
  return stc_sums_.at(i_stc);
}

int HcalBackplaneECONTPairSample::max_tc(int i_stc) const {
  return max_tc_.at(i_stc);
}

}  // namespace pflib::packing
