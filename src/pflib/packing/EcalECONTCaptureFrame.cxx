#include "pflib/packing/EcalECONTCaptureFrame.h"

#include "pflib/packing/DecompressAEBM.h"
#include "pflib/packing/Mask.h"

namespace pflib::packing {
void EcalECONTCaptureFrame::SingleECONTSample::from(std::span<uint32_t> data) {
  if (data.size() != 6) {
    pflib_log(error)
        << "Data received by EcalECONTCaptureFrame::SingleECONTSample is not length 6, it is length "
        << data.size();
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

int EcalECONTCaptureFrame::SingleECONTSample::bx() const { return bx_; }

int EcalECONTCaptureFrame::SingleECONTSample::sum() const {
  return decompressAEBM<5, 3>(encoded_sum());
}

int EcalECONTCaptureFrame::SingleECONTSample::encoded_sum() const {
  return sum_;
}

std::bitset<48> EcalECONTCaptureFrame::SingleECONTSample::ch_map() const {
  return tc_ch_map_;
}

int EcalECONTCaptureFrame::SingleECONTSample::tc(int i_tc) const {
  return max_tc_.at(i_tc);
}

EcalECONTCaptureFrame::EcalECONTCaptureFrame(std::span<uint32_t> data) {
  from(data);
}

void EcalECONTCaptureFrame::from(std::span<uint32_t> data) {
  header_.from(data);

  if (header_.length() > data.size()) {
    pflib_log(error) << "Truncated packet, "
                     << "header reports a length of '" << header_.length()
                     << " but we only recieved " << data.size() << " words";
  }

  if (header_.n_links() % 3 != 0) {
    pflib_log(error) << "Truncated packet? Number of links captured ("
                     << header_.n_links() << ") is not a multiple of 3.";
  }

  // each ECON-T in the Ecal outputs 6 eTx
  int n_econt = header_.n_links() / 6;
  pflib_log(debug) << "found " << header_.n_links() << " which means "
                   << n_econt << " ECON-Ts";
  pflib_log(debug) << " each with " << header_.n_samples() << " samples";

  /**
   * and then, with ECON-T configured to be STC4 with
   * 5M+4E encoding, we then see six 32b words for each
   * sample (i.e. size should equal 2 + 6 * samples).
   */
  samples_.resize(n_econt);
  for (int i_econt{0}; i_econt < n_econt; i_econt++) {
    samples_[i_econt].resize(header_.n_samples());
  }
  for (int i_sample{0}; i_sample < header_.n_samples(); i_sample++) {
    for (int i_econt{0}; i_econt < n_econt; i_econt++) {
      samples_[i_econt][i_sample].from(
          data.subspan(2 + 6 * (i_sample + i_econt), 6));
    }
  }
}

Reader& EcalECONTCaptureFrame::read(Reader& r) {
  // peak at first 2 to get the header
  std::vector<uint32_t> words;
  if (not r.read(words, 2)) {
    pflib_log(error)
        << "Need at least two headers to read a EcalECONTCaptureFrame";
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

const EcalECONTCaptureFrame::SingleECONTSample& EcalECONTCaptureFrame::sample(
    std::optional<int> i_econt, std::optional<int> i_sample) const {
  return samples_.at(i_econt.value_or(0))
      .at(i_sample.value_or(header().pre_samples()));
}

int EcalECONTCaptureFrame::bx(std::optional<int> i_econt,
                              std::optional<int> i_sample) const {
  return sample(i_econt, i_sample).bx();
}

int EcalECONTCaptureFrame::encoded_sum(std::optional<int> i_econt,
                                       std::optional<int> i_sample) const {
  return sample(i_econt, i_sample).encoded_sum();
}

int EcalECONTCaptureFrame::sum(std::optional<int> i_econt,
                               std::optional<int> i_sample) const {
  return sample(i_econt, i_sample).sum();
}

std::bitset<48> EcalECONTCaptureFrame::ch_map(std::optional<int> i_econt,
                                              std::optional<int> i_sample) const {
  return sample(i_econt, i_sample).ch_map();
}

int EcalECONTCaptureFrame::tc(int i_tc, std::optional<int> i_econt,
                                  std::optional<int> i_sample) const {
  return sample(i_econt, i_sample).tc(i_tc);
}

const ECONTCaptureHeader& EcalECONTCaptureFrame::header() const {
  return header_;
}

int EcalECONTCaptureFrame::n_econt() const { return header().n_links() / 6; }

int EcalECONTCaptureFrame::length() const { return header().length(); }

int EcalECONTCaptureFrame::version() const { return header().version(); }

int EcalECONTCaptureFrame::econ_id() const { return header().econ_id(); }

int EcalECONTCaptureFrame::pre_samples() const {
  return header().pre_samples();
}

std::size_t EcalECONTCaptureFrame::n_samples() const {
  return header().n_samples();
}

}  // namespace pflib::packing
