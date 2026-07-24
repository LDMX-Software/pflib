#include "pflib/packing/TrigAlgoOutput.h"

namespace pflib::packing {
const TrigAlgoOutput::SingleBXOutput& TrigAlgoOutput::sample(
    std::optional<int> i_sample) const {
  return samples_.at(i_sample.value_or(header_.pre_samples()));
}

TrigAlgoOutput::TrigAlgoOutput(std::span<uint32_t> data) { from(data); }

void TrigAlgoOutput::from(std::span<uint32_t> data) {
  header_.from(data);
  if (header_.length() > data.size()) {
    pflib_log(error) << "Header reports a length of " << header_.length()
                     << " but we only recieved " << data.size() << " words";
    return;
  }
  samples_.resize(header_.n_samples());
  for (int i_sample{0}; i_sample < samples_.size(); i_sample++) {
    samples_[i_sample].is_high_peak_ = ((data[i_sample + 2] >> 8) & 0xff);
    samples_[i_sample].trigger_ = ((data[i_sample + 2] & 0x1) == 1);
  }
}

Reader& TrigAlgoOutput::read(Reader& r) {
  std::vector<uint32_t> words;
  if (not r.read(words, 2)) {
    pflib_log(error)
        << "Need at least 2 32b header words to read a TrigAlgoOutput";
    return r;
  }
  header_.from(words);
  if (not r.read(words, header_.length() - 2, 2)) {
    pflib_log(error) << "Packet truncated, could not read entire length of "
                        "TrigAlgoOutput words";
    return r;
  }
  // reparses header but avoids copying the sample decoding code
  from(words);
  return r;
}

std::size_t TrigAlgoOutput::n_samples() const { return samples_.size(); }

const ECONTCaptureHeader& TrigAlgoOutput::header() const { return header_; }

int TrigAlgoOutput::length() const { return header_.length(); }

bool TrigAlgoOutput::is_high_peak(int i_stc,
                                  std::optional<int> i_sample) const {
  return sample(i_sample).is_high_peak_.test(i_stc);
}

bool TrigAlgoOutput::trigger(std::optional<int> i_sample) const {
  return sample(i_sample).trigger_;
}
}  // namespace pflib::packing
