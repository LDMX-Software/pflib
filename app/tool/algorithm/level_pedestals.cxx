#include "level_pedestals.h"

#include "../daq_run.h"
#include "../tasks/level_pedestals.h"
#include "pflib/utility/median.h"
#include "pflib/utility/string_format.h"

namespace pflib::algorithm {

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
    int i_roc, const pflib::packing::SingleECONDRocErxMapping& mapping,
    const std::vector<pflib::packing::MultiSampleECONDEventPacket>& data) {
  std::array<int, 72> medians;
  /// reserve a vector of the appropriate size to avoid repeating allocation
  /// time for all 72 channels
  std::vector<int> adcs(data.size());
  for (int ch{0}; ch < 72; ch++) {
    for (std::size_t i{0}; i < adcs.size(); i++) {
      auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, ch);
      adcs[i] = data[i].soi().channel(i_erx, i_ch).adc();
    }
    medians[ch] = pflib::utility::median(adcs);
  }
  return medians;
}

std::map<int, std::map<std::string, std::map<std::string, uint64_t>>>
level_pedestals(Target* tgt) {
  static auto the_log_{::pflib::logging::get("level_pedestals")};

  /// do three runs of 100 samples each to have well defined pedestals
  static const std::size_t n_events = 100;

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);
  pflib_log(info) << "Using DAQ format mode: "
                  << static_cast<int>(pftool::state.daq_format_mode);

  // each output stream ("half" of a ROC) has its own target pedestal value
  std::map<int, std::array<int, 2>> target;
  // each channel has measurements at different parameter points
  std::map<int, std::array<int, 72>> baseline, highend, lowend;

  DecodeAndBuffer buffer{n_events, tgt->nrocs() * 2};

  {  // baseline run scope
    pflib_log(info) << "100 event baseline run";
    std::map<std::string, std::map<std::string, uint64_t>> parameters;
    for (int ch{0}; ch < 72; ch++) {
      std::string ch_str{"CH_" + std::to_string(ch)};
      parameters[ch_str]["SIGN_DAC"] = 0;
      parameters[ch_str]["DACB"] = 0;
      parameters[ch_str]["TRIM_INV"] = 0;
    }

    std::map<int, std::map<int, std::map<int, uint8_t>>> prior_registers;
    for (int i_roc : tgt->roc_ids()) {
      prior_registers[i_roc] = tgt->roc(i_roc).applyParameters(parameters);
    }

    daq_run(tgt, "PEDESTAL", buffer, n_events, 100);
    pflib_log(trace) << "baseline run done, getting channel medians";
    std::map<int, std::array<int, 72>> medians;
    for (int i_roc : tgt->roc_ids()) {
      medians[i_roc] =
          get_adc_medians(i_roc, tgt->getRocErxMapping(), buffer.get_buffer());
    }
    baseline = medians;
    pflib_log(trace) << "got channel medians, getting link medians";
    for (int i_half{0}; i_half < 2; i_half++) {
      for (int i_roc : tgt->roc_ids()) {
        auto start{medians.at(i_roc).begin() + 36 * i_half};
        auto end{start + 36};
        auto halfway{start + 18};
        std::nth_element(start, halfway, end);
        target[i_roc][i_half] = *halfway;
      }
    }
    pflib_log(trace) << "got medians per half";

    for (int i_roc : tgt->roc_ids()) {
      tgt->roc(i_roc).setRegisters(prior_registers.at(i_roc));
    }
  }

  {  // highend run scope
    pflib_log(info) << "100 event highend run";
    std::map<std::string, std::map<std::string, uint64_t>> parameters;
    for (int ch{0}; ch < 72; ch++) {
      std::string ch_str{"CH_" + std::to_string(ch)};
      parameters[ch_str]["SIGN_DAC"] = 0;
      parameters[ch_str]["DACB"] = 0;
      parameters[ch_str]["TRIM_INV"] = 63;
    }

    std::map<int, std::map<int, std::map<int, uint8_t>>> prior_registers;
    for (int i_roc : tgt->roc_ids()) {
      prior_registers[i_roc] = tgt->roc(i_roc).applyParameters(parameters);
    }

    daq_run(tgt, "PEDESTAL", buffer, n_events, 100);

    for (int i_roc : tgt->roc_ids()) {
      highend[i_roc] =
          get_adc_medians(i_roc, tgt->getRocErxMapping(), buffer.get_buffer());
    }

    for (int i_roc : tgt->roc_ids()) {
      tgt->roc(i_roc).setRegisters(prior_registers.at(i_roc));
    }
  }

  {  // lowend run
    pflib_log(info) << "100 event lowend run";
    std::map<std::string, std::map<std::string, uint64_t>> parameters;
    for (int ch{0}; ch < 72; ch++) {
      std::string ch_str{"CH_" + std::to_string(ch)};
      parameters[ch_str]["SIGN_DAC"] = 1;
      parameters[ch_str]["DACB"] = 31;
      parameters[ch_str]["TRIM_INV"] = 0;
    }

    std::map<int, std::map<int, std::map<int, uint8_t>>> prior_registers;
    for (int i_roc : tgt->roc_ids()) {
      prior_registers[i_roc] = tgt->roc(i_roc).applyParameters(parameters);
    }

    daq_run(tgt, "PEDESTAL", buffer, n_events, 100);

    for (int i_roc : tgt->roc_ids()) {
      lowend[i_roc] =
          get_adc_medians(i_roc, tgt->getRocErxMapping(), buffer.get_buffer());
    }

    for (int i_roc : tgt->roc_ids()) {
      tgt->roc(i_roc).setRegisters(prior_registers.at(i_roc));
    }
  }

  pflib_log(info) << "sample collections done, deducing settings";
  std::map<int, std::map<std::string, std::map<std::string, uint64_t>>>
      settings;
  for (int i_roc : tgt->roc_ids()) {
    for (int ch{0}; ch < 72; ch++) {
      std::string page{pflib::utility::string_format("CH_%d", ch)};
      int i_half = ch / 36;
      if (baseline[i_roc].at(ch) < target[i_roc].at(i_half)) {
        pflib_log(debug) << "Channel " << ch
                         << " is below target, setting TRIM_INV";
        double scale = static_cast<double>(target[i_roc].at(i_half) -
                                           baseline[i_roc].at(ch)) /
                       (highend[i_roc].at(ch) - baseline[i_roc].at(ch));
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
          settings[i_roc][page]["TRIM_INV"] = 63;
          continue;
        }
        // scale is in [0,1]
        double optim = scale * 63;
        uint64_t val = static_cast<uint64_t>(optim);
        pflib_log(trace) << "Scale " << scale << " giving optimal value of "
                         << optim << " which rounds to " << val;
        settings[i_roc][page]["TRIM_INV"] = val;
      } else {
        double scale = static_cast<double>(baseline[i_roc].at(ch) -
                                           target[i_roc].at(i_half)) /
                       (baseline[i_roc].at(ch) - lowend[i_roc].at(ch));
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
          settings[i_roc][page]["SIGN_DAC"] = 1;
          settings[i_roc][page]["DACB"] = 31;
          continue;
        }
        double optim = scale * 31;
        uint64_t val = static_cast<uint64_t>(optim);
        pflib_log(trace) << "Scale " << scale << " giving optimal value of "
                         << optim << " which rounds to " << val;
        if (val == 0) {
          pflib_log(debug) << "Channel " << ch
                           << " is above target but too close to use DACB to "
                              "lower, skipping";
        } else {
          pflib_log(debug) << "Channel " << ch
                           << " is above target, setting SIGN_DAC=1 and DACB";
          settings[i_roc][page]["SIGN_DAC"] = 1;
          settings[i_roc][page]["DACB"] = val;
        }
      }
    }
  }

  return settings;
}

}  // namespace pflib::algorithm
