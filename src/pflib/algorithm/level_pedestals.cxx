#include "pflib/algorithm/level_pedestals.h"

#include "pflib/DecodeAndWrite.h"

/// probably partially replace by buffer that is in #177

class DecodeAndGetMedians : public DecodeAndWrite {
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
      std::nth_element(
          buffer_.begin(), buffer_.begin()+halfway, buffer_.end(),
          [&](
            const pflib::packing::SingleROCEventPacket& lhs,
            const pflib::packing::SingleROCEventPacket& rhs
          ) {
            return lhs.channel(ch).adc() < rhs.channel(ch).adc();
          }
      );
      medians[ch] = buffer_[halfway];
    }
    return medians;
  }
};

namespace pflib::algorithm {

void level_pedestals(Target* tgt, int iroc, std::string type_version) {
  /// do three runs of 10k samples each to have well defined pedestals
  static const std::size_t n_events = 10_000;

  auto roc = tgt->hcal().roc(iroc, type_version);

  tgt->setup_run(1, DAG_FORMAT_SIMPLEROC, 1);

  std::array<int, 2> target;
  std::array<int, 72> baseline, highend, lowend;
  DecodeAndGetMedians buffer{n_events};
  
  { // baseline run
    auto roc.testParameters()
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
    auto roc.testParameters()
      .add_all_channels("SIGN_DAC", 0)
      .add_all_channels("DACB", 0)
      .add_all_channels("TRIM_INV", 63)
      .apply();
    tgt->daq_run("PEDESTAL", buffer, n_events, 100);
    highend = buffer.get_medians();
  }

  { // lowend run
    auto roc.testParameters()
      .add_all_channels("SIGN_DAC", 1)
      .add_all_channels("DACB", 31)
      .add_all_channels("TRIM_INV", 0)
      .apply();
    tgt->daq_run("PEDESTAL", buffer, n_events, 100);
    lowend = buffer.get_medians();
  }

  std::map<std::string, std::map<std::string, int>> settings;
  for (int ch{0}; ch < 72; ch++) {
    std::string page{string_format("CH_%d", ch)};
    int i_link = ch / 36;
    if (baseline.at(ch) < target.at(i_link)) {
      settings[page]["TRIM_INV"] = (target.at(i_link) - baseline.at(ch))/(highend.at(ch) - baseline.at(ch)) * 63;
    } else {
      settings[page]["SIGN_DACB"] = 1;
      settings[page]["DACB"] = (baseline.at(ch) - target.at(i_link))/(baseline.at(ch) - lowend.at(ch)) * 31;
    }
  }

  return settings;
}

}
