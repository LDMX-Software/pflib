#include "pflib/packing/TrigPathCaptureFrame.h"

namespace pflib::packing {
template<class T>
TrigPathCaptureFrame<T>::TrigPathCaptureFrame(std::span<uint32_t> data) {
  from(data);
}
template<class T>
Reader& TrigPathCaptureFrame<T>::read(Reader& r) {
  // peak at first 2 to get the header
  std::vector<uint32_t> words;
  if (not r.read(words, 2)) {
    pflib_log(error)
        << "Need at least two headers to read a TrigPathCaptureFrame";
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
  from(words);
  return r;
}

template<class T>
void TrigPathCaptureFrame<T>::from(std::span<uint32_t> data) {
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

  pflib_log(debug) << "found " << header_.n_links() << " links each with "
                   << header_.n_samples() << " samples";

  samples_.resize(header_.n_samples());
  for (int i_sample{0}; i_sample < samples_.size(); i_sample++) {
    samples_[i_sample].from(data.subspan(2 + T::WORD_LEN * i_sample, T::WORD_LEN));
  }
}

template <class T>
const T& TrigPathCaptureFrame<T>::sample(std::optional<int> i_sample) const {
  return samples_.at(i_sample.value_or(header().pre_samples()));
}

template<class T>
const ECONTCaptureHeader& TrigPathCaptureFrame<T>::header() const {
  return header_;
}

}

#include "pflib/packing/HcalECONTCaptureFrame.h"
#include "pflib/packing/EcalECONTCaptureFrame.h"
// explicit template instantiation to only compile once here
template class pflib::packing::TrigPathCaptureFrame<pflib::packing::HcalBackplaneECONTPairSample>;
template class pflib::packing::TrigPathCaptureFrame<pflib::packing::EcalECONTSample>;
