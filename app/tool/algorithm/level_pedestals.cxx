#include "level_pedestals.h"

#include "../daq_run.h"
#include "../tasks/level_pedestals.h"
#include "pflib/utility/median.h"
#include "pflib/utility/string_format.h"

namespace pflib::algorithm {

/**
 * Retrieve the ADC sample for the input channel from the input event packet
 */
static int get_adc(const pflib::packing::MultiSampleECONDEventPacket& p,
                   int ch) {
  // Use link specific channel calculation
  // TODO: 348
  // Use the "Sample Of Interest" inside the EventPacket
  // TODO this is only true if we only have one ROC's channels enabled
  //      in the ECON-D. In the more realistic case, we should get the
  //      link indices depending on which ROC we are aligning
  int i_link = ch / 36;  // 0 or 1
  int i_ch = ch % 36;    // 0 - 35

  // ECONDEventPacket.h defines channel differently to SingleROCEventPacket.h
  // because it can have more than 2 links readout
  return p.samples[p.i_soi].channel(i_link, i_ch).adc();
}

/**
 * get the medians of the channel ADC values
 *
 * This may be helpful in some other contexts, but since it depends on the
 * packing library it cannot go into utility. Just keeping it here for now,
 * maybe move it into its own header/impl in algorithm.
 *
 * @param[in] data buffer of single-roc packet data
 * @return array of channel ADC values
 *
 * @note We assume the caller knows what they are doing.
 * Calib and Common Mode channels are ignored.
 * TOT/TOA and the sample Tp/Tc flags are ignored.
 */
static std::array<int, 72> get_adc_medians(
    const std::vector<pflib::packing::MultiSampleECONDEventPacket>& data) {
  std::array<int, 72> medians;
  /// reserve a vector of the appropriate size to avoid repeating allocation
  /// time for all 72 channels
  std::vector<int> adcs(data.size());
  for (int ch{0}; ch < 72; ch++) {
    for (std::size_t i{0}; i < adcs.size(); i++) {
      adcs[i] = get_adc(data[i], ch);
    }
    medians[ch] = pflib::utility::median(adcs);
  }
  return medians;
}

std::map<std::string, std::map<std::string, uint64_t>> level_pedestals(
    Target* tgt, ROC roc) {
  static auto the_log_{::pflib::logging::get("level_pedestals")};

  /// do three runs of 100 samples each to have well defined pedestals
  static const std::size_t n_events = 100;

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);
  pflib_log(info) << "Using DAQ format mode: "
                  << static_cast<int>(pftool::state.daq_format_mode);

  std::array<int, 2> target;
  std::array<int, 72> baseline, highend, lowend;

  /// TODO for multi-ROC set ups, we could dynamically determine the number
  //       of ROCs and the number of channels from the Target
  DecodeAndBuffer buffer{n_events, 2};

  {  // baseline run scope
    pflib_log(info) << "100 event baseline run";
    auto test_handle_builder = roc.testParameters();
    for (int ch{0}; ch < 72; ch++) {
      std::string page{"CH_" + std::to_string(ch)};
      test_handle_builder.add(page, "SIGN_DAC", 0);
      test_handle_builder.add(page, "DACB", 0);
      test_handle_builder.add(page, "TRIM_INV", 0);
    }
    auto test_handle = test_handle_builder.apply();
    daq_run(tgt, "PEDESTAL", buffer, n_events, 100);
    pflib_log(trace) << "baseline run done, getting channel medians";
    auto medians = get_adc_medians(buffer.get_buffer());
    baseline = medians;
    pflib_log(trace) << "got channel medians, getting link medians";
    for (int i_link{0}; i_link < 2; i_link++) {
      auto start{medians.begin() + 36 * i_link};
      auto end{start + 36};
      auto halfway{start + 18};
      std::nth_element(start, halfway, end);
      target[i_link] = *halfway;
    }
    pflib_log(trace) << "got link medians";
  }

  {  // highend run scope
    pflib_log(info) << "100 event highend run";
    auto test_handle_builder = roc.testParameters();
    for (int ch{0}; ch < 72; ch++) {
      std::string page{"CH_" + std::to_string(ch)};
      test_handle_builder.add(page, "SIGN_DAC", 0);
      test_handle_builder.add(page, "DACB", 0);
      test_handle_builder.add(page, "TRIM_INV", 63);
    }
    auto test_handle = test_handle_builder.apply();
    daq_run(tgt, "PEDESTAL", buffer, n_events, 100);
    highend = get_adc_medians(buffer.get_buffer());
  }

  {  // lowend run
    pflib_log(info) << "100 event lowend run";
    auto test_handle_builder = roc.testParameters();
    for (int ch{0}; ch < 72; ch++) {
      std::string page{"CH_" + std::to_string(ch)};
      test_handle_builder.add(page, "SIGN_DAC", 1);
      test_handle_builder.add(page, "DACB", 31);
      test_handle_builder.add(page, "TRIM_INV", 0);
    }
    auto test_handle = test_handle_builder.apply();
    daq_run(tgt, "PEDESTAL", buffer, n_events, 100);
    lowend = get_adc_medians(buffer.get_buffer());
  }

  pflib_log(info) << "sample collections done, deducing settings";
  std::map<std::string, std::map<std::string, uint64_t>> settings;
  for (int ch{0}; ch < 72; ch++) {
    std::string page{pflib::utility::string_format("CH_%d", ch)};
    int i_link = ch / 36;
    if (baseline.at(ch) < target.at(i_link)) {
      pflib_log(debug) << "Channel " << ch
                       << " is below target, setting TRIM_INV";
      double scale = static_cast<double>(target.at(i_link) - baseline.at(ch)) /
                     (highend.at(ch) - baseline.at(ch));
      if (scale < 0) {
        pflib_log(warn) << "Channel " << ch
                        << " is below target but increasing TRIM_INV made it "
                           "lower??? Skipping...";
        continue;
      }
      if (scale > 1) {
        pflib_log(warn) << "Channel " << ch
                        << " is so far below target that we cannot increase "
                           "TRIM_INV enough."
                        << " Setting TRIM_INV to its maximum.";
        settings[page]["TRIM_INV"] = 63;
        continue;
      }
      // scale is in [0,1]
      double optim = scale * 63;
      uint64_t val = static_cast<uint64_t>(optim);
      pflib_log(trace) << "Scale " << scale << " giving optimal value of "
                       << optim << " which rounds to " << val;
      settings[page]["TRIM_INV"] = val;
    } else {
      double scale = static_cast<double>(baseline.at(ch) - target.at(i_link)) /
                     (baseline.at(ch) - lowend.at(ch));
      if (scale < 0) {
        pflib_log(warn) << "Channel " << ch
                        << " is above target but using SIGN_DAC=1 and "
                           "increasing DACB made it higher??? Skipping...";
        continue;
      }
      if (scale > 1) {
        pflib_log(warn)
            << "Channel " << ch
            << " is so far above target that we cannot lower it enough."
            << " Setting SIGN_DAC=1 and DACB to its maximum.";
        settings[page]["SIGN_DAC"] = 1;
        settings[page]["DACB"] = 31;
        continue;
      }
      double optim = scale * 31;
      uint64_t val = static_cast<uint64_t>(optim);
      pflib_log(trace) << "Scale " << scale << " giving optimal value of "
                       << optim << " which rounds to " << val;
      if (val == 0) {
        pflib_log(debug)
            << "Channel " << ch
            << " is above target but too close to use DACB to lower, skipping";
      } else {
        pflib_log(debug) << "Channel " << ch
                         << " is above target, setting SIGN_DAC=1 and DACB";
        settings[page]["SIGN_DAC"] = 1;
        settings[page]["DACB"] = val;
      }
    }
  }

  return settings;
}

}  // namespace pflib::algorithm
