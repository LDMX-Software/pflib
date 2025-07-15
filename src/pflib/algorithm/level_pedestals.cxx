#include "pflib/algorithm/level_pedestals.h"
#include "pflib/utility/string_format.h"

#include "pflib/DecodeAndWrite.h"

/// probably partially replace by buffer that is in #177

class DecodeAndGetMedians : public pflib::DecodeAndWrite {
  std::vector<pflib::packing::SingleROCEventPacket> buffer_;
 public:
  DecodeAndGetMedians(std::size_t n) : DecodeAndWrite() {
    buffer_.reserve(n);
  }
  virtual ~DecodeAndGetMedians() = default;
  virtual void start_run() final {
    buffer_.clear();
  }
  virtual void write_event(const pflib::packing::SingleROCEventPacket &ep) final {
    if (buffer_.size() == buffer_.capacity()) {
      PFEXCEPTION_RAISE("BuffOverflow", "Attempting to fill buffer past its capacity.");
    }
    buffer_.push_back(ep);
  }
  const auto& get_buffer() const { return buffer_; }
  std::array<int, 72> get_medians() const {
    std::size_t halfway{buffer_.size() / 2};
    std::array<int, 72> medians;
    for (int ch{0}; ch < 72; ch++) {
      std::vector<int> adcs(buffer_.size());
      for (std::size_t i{0}; i < adcs.size(); i++) {
        adcs[i] = buffer_[i].channel(ch).adc();
      }
      std::nth_element(adcs.begin(), adcs.begin()+halfway, adcs.end());
      medians[ch] = adcs[halfway];
    }
    return medians;
  }
};

namespace pflib::algorithm {

std::map<std::string, std::map<std::string, int>>
level_pedestals(Target* tgt, ROC roc) {
  static auto the_log_{::pflib::logging::get("level_pedestals")};

  /// do three runs of 10k samples each to have well defined pedestals
  static const std::size_t n_events = 100;

  tgt->setup_run(1, 1 /*DAQ_FORMAT_SIMPLEROC*/, 1);

  std::array<int, 2> target;
  std::array<int, 72> baseline, highend, lowend;
  DecodeAndGetMedians buffer{n_events};
  
  { // baseline run
    pflib_log(info) << "100 event baseline run"
    auto test_handle = roc.testParameters()
      .add_all_channels("SIGN_DAC", 0)
      .add_all_channels("DACB", 0)
      .add_all_channels("TRIM_INV", 0)
      .apply();
    tgt->daq_run("PEDESTAL", buffer, n_events, 100);
    auto medians = buffer.get_medians();
    baseline = medians;
    for (int i_link{0}; i_link < 2; i_link++) {
      auto start{medians.begin()+36*i_link};
      auto end{start+36};
      std::nth_element(start, start+18, end);
      target[i_link] = medians[36*i_link+18];
    }
  }

  { // highend run
    pflib_log(info) << "100 event highend run";
    auto test_handle = roc.testParameters()
      .add_all_channels("SIGN_DAC", 0)
      .add_all_channels("DACB", 0)
      .add_all_channels("TRIM_INV", 63)
      .apply();
    tgt->daq_run("PEDESTAL", buffer, n_events, 100);
    highend = buffer.get_medians();
  }

  { // lowend run
    pflib_log(info) << "100 event lowend run";
    auto test_handle = roc.testParameters()
      .add_all_channels("SIGN_DAC", 1)
      .add_all_channels("DACB", 31)
      .add_all_channels("TRIM_INV", 0)
      .apply();
    tgt->daq_run("PEDESTAL", buffer, n_events, 100);
    lowend = buffer.get_medians();
  }

  pflib_log(info) << "sample collections done, deducing settings";
  std::map<std::string, std::map<std::string, int>> settings;
  for (int ch{0}; ch < 72; ch++) {
    std::string page{pflib::utility::string_format("CH_%d", ch)};
    int i_link = ch / 36;
    if (baseline.at(ch) < target.at(i_link)) {
      pflib_log(debug) << "Channel " << ch << " is below target, setting TRIM_INV";
      double scale = static_cast<double>(target.at(i_link) - baseline.at(ch))/(highend.at(ch) - baseline.at(ch));
      double optim = scale*63;
      int val = static_cast<int>(optim);
      pflib_log(trace) << "Scale " << scale
                       << " giving optimal value of " << optim
                       << " which rounds to " << val;
      settings[page]["TRIM_INV"] = val;
    } else {
      pflib_log(debug) << "Channel " << ch << " is above target, setting SIGN_DACB=1 and DACB";
      settings[page]["SIGN_DAC"] = 1;
      double scale = static_cast<double>(baseline.at(ch) - target.at(i_link))/(baseline.at(ch) - lowend.at(ch));
      double optim = scale*31;
      int val = static_cast<int>(optim);
      pflib_log(trace) << "Scale " << scale
                       << " giving optimal value of " << optim
                       << " which rounds to " << val;
      settings[page]["DACB"] = val;
    }
  }

  return settings;
}

}
