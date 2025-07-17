#include "pflib/algorithm/level_pedestals.h"

#include "pflib/utility/string_format.h"
#include "pflib/utility/median.h"

#include "pflib/DecodeAndBuffer.h"

/**
 * get the medians of the channel ADC values
 *
 * This may be helpful in some other contexts, but since it depends on the packing
 * library it cannot go into utility. Just keeping it here for now, maybe move it
 * into its own header/impl in algorithm.
 *
 * @param[in] data buffer of single-roc packet data
 * @return array of channel ADC values
 *
 * @note We assume the caller knows what they are doing.
 * Calib and Common Mode channels are ignored.
 * TOT/TOA and the sample Tp/Tc flags are ignored.
 */
static std::array<int, 72> get_adc_medians(const std::vector<pflib::packing::SingleROCEventPacket> &data) {
  std::array<int, 72> medians;
  /// reserve a vector of the appropriate size to avoid repeating allocation time for all 72 channels
  std::vector<int> adcs(data.size());
  for (int ch{0}; ch < 72; ch++) {
    for (std::size_t i{0}; i < adcs.size(); i++) {
      adcs[i] = data[i].channel(ch).adc();
    }
    medians[ch] = pflib::utility::median(adcs);
  }
  return medians;
}

namespace pflib::algorithm {

std::map<std::string, std::map<std::string, int>>
level_pedestals(Target* tgt, ROC roc) {
  static auto the_log_{::pflib::logging::get("level_pedestals")};

  /// do three runs of 100 samples each to have well defined pedestals
  static const std::size_t n_events = 100;

  tgt->setup_run(1, Target::DaqFormat::SIMPLEROC, 1);

  std::array<int, 2> target;
  std::array<int, 72> baseline, highend, lowend;
  DecodeAndBuffer buffer{n_events};

  // for future devs:
  //   I do this weird extra brackets to limit the scope
  //   of the test_handle object and force it to destruct
  //   after the run is over (unsetting the parameters).
  
  { // baseline run
    pflib_log(info) << "100 event baseline run";
    auto test_handle = roc.testParameters()
      .add_all_channels("SIGN_DAC", 0)
      .add_all_channels("DACB", 0)
      .add_all_channels("TRIM_INV", 0)
      .apply();
    tgt->daq_run("PEDESTAL", buffer, n_events, 100);
    auto medians = get_adc_medians(buffer.get_buffer());
    baseline = medians;
    for (int i_link{0}; i_link < 2; i_link++) {
      auto start{medians.begin()+36*i_link};
      auto end{start+36};
      auto halfway{start+18};
      std::nth_element(start, halfway, end);
      target[i_link] = *halfway;
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
    highend = get_adc_medians(buffer.get_buffer());
  }

  { // lowend run
    pflib_log(info) << "100 event lowend run";
    auto test_handle = roc.testParameters()
      .add_all_channels("SIGN_DAC", 1)
      .add_all_channels("DACB", 31)
      .add_all_channels("TRIM_INV", 0)
      .apply();
    tgt->daq_run("PEDESTAL", buffer, n_events, 100);
    lowend = get_adc_medians(buffer.get_buffer());
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
      pflib_log(debug) << "Channel " << ch << " is above target, setting SIGN_DAC=1 and DACB";
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
