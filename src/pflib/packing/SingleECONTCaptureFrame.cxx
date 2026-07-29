#include "pflib/packing/SingleECONTCaptureFrame.h"

#include "pflib/packing/Mask.h"
#include "pflib/packing/DecodeAEBM.h"

namespace pflib::packing {
void SingleECONTCaptureFrame::SingleECONTSample::from(
    std::span<uint32_t> data) {
  if (data.size() != 3) {
    pflib_log(error)
        << "Data received by SingleECONTSample is not length 3, it is length "
        << data.size();
    return;
  }
  bx_ = ((data[0] >> 28) & mask<4>);
  for (int i{0}; i < N_STC; i++) {
    // max is all within the first 32b word even if there
    // are more eTx
    max_tc_[i] = ((data[0] >> (26 - 2 * i)) & mask<2>); 
  }

  // just hardcoding 3 eTx for now
  stc_sums_[0] = ((data[0] >> 3) & mask<9>);
  stc_sums_[1] = (((data[0] & mask<3>) << 6) | ((data[1] >> (32-6)) & mask<6>));
  stc_sums_[2] = ((data[1] >> 17) & mask<9>);
  stc_sums_[3] = ((data[1] >> 8) & mask<9>);
  stc_sums_[4] = (((data[1] & mask<8>) << 1) | ((data[2] >> 31) & mask<1>));
  stc_sums_[5] = ((data[2] >> 22) & mask<9>);
  stc_sums_[6] = ((data[2] >> 13) & mask<9>);
  stc_sums_[7] = ((data[2] >> 4) & mask<9>);
}

int SingleECONTCaptureFrame::SingleECONTSample::bx() const { return bx_; }

int SingleECONTCaptureFrame::SingleECONTSample::stc_sum(int i_stc) const {
  return decodeAEBM<5, 4>(encoded_stc_sum(i_stc));
}

int SingleECONTCaptureFrame::SingleECONTSample::encoded_stc_sum(int i_stc) const {
  return stc_sums_.at(i_stc);
}

int SingleECONTCaptureFrame::SingleECONTSample::max_tc(int i_stc) const {
  return max_tc_.at(i_stc);
}

SingleECONTCaptureFrame::SingleECONTCaptureFrame(std::span<uint32_t> data) {
  from(data);
}

void SingleECONTCaptureFrame::from(std::span<uint32_t> data) {
  header_.from(data);

  if (header_.length() > data.size()) {
    pflib_log(error) << "Truncated packet, "
                     << "header reports a length of '" << header_.length()
                     << " but we only recieved " << data.size() << " words";
  }

  /**
   * and then, with ECON-T configured to be STC4 with
   * 5M+4E encoding, we then see three 32b words for each
   * sample (i.e. size should equal 2 + 3 * samples).
   */
  samples_.resize(header_.n_samples());
  for (int i_sample{0}; i_sample < samples_.size(); i_sample++) {
    samples_[i_sample].from(data.subspan(2 + 3 * i_sample, 3));
  }
}

Reader& SingleECONTCaptureFrame::read(Reader& r) {
  // peak at first 2 to get the header
  std::vector<uint32_t> words;
  if (not r.read(words, 2)) {
    pflib_log(error)
        << "Need at least two headers to read a SingleECONTCaptureFrame";
    return r;
  }

  header_.from(words);

  if (not r.read(words, header_.length() - 2, 2)) {
    pflib_log(error) << "Truncated event packet, could not read all of the "
                        "words listed in header length";
    return r;
  }

  // this re-parses the header words but
  // i like avoiding copying the sample deduction code
  // especially since it could get more complicated
  // if we want to include decoding BC algorithm
  // in addition to the STC algorithm
  from(words);
  return r;
}

const SingleECONTCaptureFrame::SingleECONTSample&
SingleECONTCaptureFrame::sample(std::optional<int> i_sample) const {
  return samples_.at(i_sample.value_or(header().pre_samples()));
}

int SingleECONTCaptureFrame::bx(std::optional<int> i_sample) const {
  return sample(i_sample).bx();
}

int SingleECONTCaptureFrame::encoded_stc_sum(int i_stc,
                                     std::optional<int> i_sample) const {
  return sample(i_sample).encoded_stc_sum(i_stc);
}

int SingleECONTCaptureFrame::stc_sum(int i_stc,
                                     std::optional<int> i_sample) const {
  return sample(i_sample).stc_sum(i_stc);
}

int SingleECONTCaptureFrame::max_tc(int i_stc,
                                    std::optional<int> i_sample) const {
  return sample(i_sample).max_tc(i_stc);
}

const ECONTCaptureHeader& SingleECONTCaptureFrame::header() const {
  return header_;
}

int SingleECONTCaptureFrame::length() const { return header().length(); }

int SingleECONTCaptureFrame::version() const { return header().version(); }

int SingleECONTCaptureFrame::econ_id() const { return header().econ_id(); }

int SingleECONTCaptureFrame::pre_samples() const {
  return header().pre_samples();
}

std::size_t SingleECONTCaptureFrame::n_samples() const {
  return samples_.size();
}

}  // namespace pflib::packing
