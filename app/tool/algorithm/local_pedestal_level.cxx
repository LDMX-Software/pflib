#include "local_pedestal_level.h"

#include <iostream>

#include "../SpaceSeparated.h"
#include "../daq_run.h"
#include "../tasks/local_pedestal_level.h"
#include "pflib/utility/median.h"
#include "pflib/utility/stdev.h"
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

static std::array<double, 72> get_adc_stdevs(
    int i_roc, const pflib::packing::SingleECONDRocErxMapping& mapping,
    const std::vector<pflib::packing::MultiSampleECONDEventPacket>& data) {
  std::array<double, 72> stdevs;
  /// reserve a vector of the appropriate size to avoid repeating allocation
  /// time for all 72 channels
  std::vector<int> adcs(data.size());
  for (int ch{0}; ch < 72; ch++) {
    for (std::size_t i{0}; i < adcs.size(); i++) {
      auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, ch);
      adcs[i] = data[i].soi().channel(i_erx, i_ch).adc();
    }
    stdevs[ch] = pflib::utility::stdev(adcs);
  }
  return stdevs;
}

/**
 * maximum ADC standard deviation to consider a channel as broken
 *
 * normal channels have electronic noise at least on the order of 1 ADC,
 * so the standard deviation of many ADC samples should be 1 ADC or greater.
 * We consider a channel to be broken (or disabled) if the standard deviation
 * is less than 0.1.
 */
static const double MAX_BROKEN_ADC_STD = 0.1;

std::map<int, std::map<std::string, std::map<std::string, uint64_t>>>
local_pedestal_level(Target* tgt) {
  static auto the_log_{::pflib::logging::get("local_pedestal_level")};

  /// do three runs of 100 samples each to have well defined pedestals
  static const std::size_t n_events = 100;

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  // each output stream ("half" of a ROC) has its own target pedestal value
  std::map<int, std::array<int, 2>> target;
  // each channel has measurements at different parameter points
  std::map<int, std::array<int, 72>> baseline, highend, lowend;
  // will fill with standard deviations so we can ommit dead channels
  std::map<int, std::array<double, 72>> basedevs;
  // include all zero parmeter run values since hcal used DACB and SIGN_DAC
  std::map<int, std::array<int, 72>> allzero;

  // ECal channels freeze (constant ADC) for TRIM_INV above about 50
  const int trim_inv_max = pftool::state.readout_config_is_hcal() ? 63 : 48;

  DecodeAndBuffer buffer{n_events, tgt->nrocs() * 2};

  {  // baseline run scope
    pflib_log(info) << "100 event baseline run";
    std::map<std::string, std::map<std::string, uint64_t>> parameters;
    for (int ch{0}; ch < 72; ch++) {
      std::string ch_str{"CH_" + std::to_string(ch)};
      if (pftool::state.readout_config_is_hcal()) {
        parameters[ch_str]["SIGN_DAC"] = 0;
        parameters[ch_str]["DACB"] = 0;
        parameters[ch_str]["TRIM_INV"] = 32;
      } else {
        parameters[ch_str]["TRIM_INV"] = 32;
      }
    }
    auto test_params = tgt->tempApplyAllROCs(parameters);

    daq_run(tgt, "PEDESTAL", buffer, n_events, pftool::state.daq_rate);
    pflib_log(trace) << "baseline run done, getting channel medians";
    std::map<int, std::array<int, 72>> medians;
    for (int i_roc : tgt->roc_ids()) {
      medians[i_roc] =
          get_adc_medians(i_roc, tgt->getRocErxMapping(), buffer.get_buffer());
      basedevs[i_roc] =
          get_adc_stdevs(i_roc, tgt->getRocErxMapping(), buffer.get_buffer());
    }
    baseline = medians;
    pflib_log(trace) << "got channel medians, getting link medians";
    for (int i_half{0}; i_half < 2; i_half++) {
      for (int i_roc : tgt->roc_ids()) {
        std::vector<int> half_medians;
        // only include non-broken channels when determining target median
        half_medians.reserve(36);
        for (int i_ch{0}; i_ch < 36; i_ch++) {
          if (basedevs[i_roc][i_ch + 36 * i_half] > MAX_BROKEN_ADC_STD) {
            half_medians.push_back(medians[i_roc][i_ch + 36 * i_half]);
          }
        }
        if (half_medians.empty()) {
          pflib_log(warn) << "no channels in ROC" << i_roc << " half " << i_half
                          << " are enabled, they all should be skipped";
          target[i_roc][i_half] = 1023;
        } else {
          target[i_roc][i_half] = pflib::utility::median(half_medians);
        }
      }
    }
    pflib_log(trace) << "got medians per half";
  }

  if (pftool::state.readout_config_is_hcal()) {
    // allzero run scope (SIGN_DAC = DACB = TRIM_INV = 0)
    pflib_log(info) << "100 event all-zero-parameters run";
    std::map<std::string, std::map<std::string, uint64_t>> parameters;
    for (int ch{0}; ch < 72; ch++) {
      std::string ch_str{"CH_" + std::to_string(ch)};
      parameters[ch_str]["SIGN_DAC"] = 0;
      parameters[ch_str]["DACB"] = 0;
      parameters[ch_str]["TRIM_INV"] = 0;
    }
    auto test_params = tgt->tempApplyAllROCs(parameters);

    daq_run(tgt, "PEDESTAL", buffer, n_events, pftool::state.daq_rate);
    pflib_log(trace) << "all-zero-params run done, getting channel medians";
    for (int i_roc : tgt->roc_ids()) {
      allzero[i_roc] =
          get_adc_medians(i_roc, tgt->getRocErxMapping(), buffer.get_buffer());
    }
  }

  {  // highend run scope
    pflib_log(info) << "100 event highend run";
    std::map<std::string, std::map<std::string, uint64_t>> parameters;
    for (int ch{0}; ch < 72; ch++) {
      std::string ch_str{"CH_" + std::to_string(ch)};
      if (pftool::state.readout_config_is_hcal()) {
        parameters[ch_str]["SIGN_DAC"] = 0;
        parameters[ch_str]["DACB"] = 0;
        parameters[ch_str]["TRIM_INV"] = trim_inv_max;
      } else {
        parameters[ch_str]["TRIM_INV"] = trim_inv_max;
      }
    }
    auto test_params = tgt->tempApplyAllROCs(parameters);

    daq_run(tgt, "PEDESTAL", buffer, n_events, pftool::state.daq_rate);

    for (int i_roc : tgt->roc_ids()) {
      highend[i_roc] =
          get_adc_medians(i_roc, tgt->getRocErxMapping(), buffer.get_buffer());
    }
  }

  {  // lowend run
    pflib_log(info) << "100 event lowend run";
    std::map<std::string, std::map<std::string, uint64_t>> parameters;
    for (int ch{0}; ch < 72; ch++) {
      std::string ch_str{"CH_" + std::to_string(ch)};
      if (pftool::state.readout_config_is_hcal()) {
        parameters[ch_str]["SIGN_DAC"] = 1;
        parameters[ch_str]["DACB"] = 31;
        parameters[ch_str]["TRIM_INV"] = 0;
      } else {
        parameters[ch_str]["TRIM_INV"] = 0;
      }
    }
    auto test_params = tgt->tempApplyAllROCs(parameters);

    daq_run(tgt, "PEDESTAL", buffer, n_events, pftool::state.daq_rate);

    for (int i_roc : tgt->roc_ids()) {
      lowend[i_roc] =
          get_adc_medians(i_roc, tgt->getRocErxMapping(), buffer.get_buffer());
    }
  }

  pflib_log(info) << "sample collections done, deducing settings";
  std::map<int, std::map<std::string, std::map<std::string, uint64_t>>>
      settings;
  std::map<int, std::vector<int>> ignored_channels;
  for (int i_roc : tgt->roc_ids()) {
    pflib_log(info) << "ROC" << i_roc << " targets by half: "
                    << SpaceSeparated(target[i_roc].begin(),
                                      target[i_roc].end());
    // ignored_channels[i_roc];
    for (int ch{0}; ch < 72; ch++) {
      if (basedevs[i_roc].at(ch) < MAX_BROKEN_ADC_STD) {
        ignored_channels[i_roc].push_back(ch);
        continue;
      }
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
          settings[i_roc][page]["TRIM_INV"] = trim_inv_max;
          continue;
        }
        // scale is in [0,1]
        double optim = 32 + (scale * (trim_inv_max - 32));
        uint64_t val = static_cast<uint64_t>(optim);
        pflib_log(trace) << "Scale " << scale << " giving optimal value of "
                         << optim << " which rounds to " << val;
        settings[i_roc][page]["TRIM_INV"] = val;
        continue;
      } else /* baseline[ch] > target[i_half]  */ {
        double scale = static_cast<double>(baseline[i_roc].at(ch) -
                                           target[i_roc].at(i_half)) /
                       (baseline[i_roc].at(ch) - lowend[i_roc].at(ch));
        if (scale < 0 && pftool::state.readout_config_is_hcal()) {
          pflib_log(warn) << "Channel " << ch
                          << " is above target but decreasing TRIM_INV "
                             "and using SIGN_DAC=1 and increasing DACB "
                             "made it higher??? Skipping...";
          continue;
        }
        if (scale < 0 && !pftool::state.readout_config_is_hcal()) {
          pflib_log(warn) << "Channel " << ch
                          << " is above target but decreasing TRIM_INV "
                             "made it higher??? Skipping...";
          continue;
        }
        if (scale > 1 && pftool::state.readout_config_is_hcal()) {
          pflib_log(warn)
              << "Channel " << ch
              << " is so far above target that we cannot lower it enough."
              << " Setting SIGN_DAC=1 and DACB to its maximumum."
              << " Setting TRIM_INV = 0.";
          settings[i_roc][page]["TRIM_INV"] = 0;
          settings[i_roc][page]["SIGN_DAC"] = 1;
          settings[i_roc][page]["DACB"] = 31;
          continue;
        }
        if (scale > 1 && !pftool::state.readout_config_is_hcal()) {
          pflib_log(warn)
              << "Channel " << ch
              << " is so far above target that we cannot lower it enough."
              << " Setting TRIM_INV = 0.";
          settings[i_roc][page]["TRIM_INV"] = 0;
          continue;
        }
        // scale is in [0,1]
        double diff = static_cast<double>(baseline[i_roc].at(ch) -
                                          target[i_roc].at(i_half));
        if (pftool::state.readout_config_is_hcal()) {
          double trim_diff = static_cast<double>(baseline[i_roc].at(ch) -
                                                 allzero[i_roc].at(ch));
          double dacb_diff =
              static_cast<double>(allzero[i_roc].at(ch) - lowend[i_roc].at(ch));
          if (trim_diff < 0 || dacb_diff < 0) {
            pflib_log(debug)
                << "Channel " << ch << " is above target but lowering TRIM_INV "
                << "or setting SIGN_DAC = 1 and raising DACB"
                << " made it higher??? Skipping...";
            continue;
          }
          if (diff <= trim_diff) {
            double optim = 32 - ((diff / trim_diff) * 32);
            uint64_t val = static_cast<uint64_t>(optim);
            pflib_log(trace) << "Scale " << (diff / trim_diff)
                             << " giving optimal value of " << optim
                             << " for TRIM_INV"
                                " which rounds to "
                             << val;
            if (val == 0) {
              pflib_log(debug)
                  << "Channel " << ch
                  << " is above target but too close to use TRIM_INV "
                     "to lower, skipping.";
            } else {
              pflib_log(debug)
                  << "Channel " << ch << " is above target, lowering TRIM_INV";
              settings[i_roc][page]["TRIM_INV"] = val;
            }
          } else /* diff > trim_diff */ {
            double optim = ((diff - trim_diff) / dacb_diff) * 31;
            uint64_t val = static_cast<uint64_t>(optim);
            pflib_log(trace) << "Scale " << ((diff - trim_diff) / dacb_diff)
                             << " giving optimal value of " << optim
                             << " for DACB"
                                " which rounds to "
                             << val;
            if (val == 0) {
              pflib_log(debug) << "Channel " << ch
                               << " is above target but too close to use DACB "
                                  "to lower. Setting TRIM_INV = 0";
              settings[i_roc][page]["TRIM_INV"] = 0;
            } else {
              pflib_log(debug) << "Channel" << ch
                               << " is above target, setting TRIM_INV = 0."
                                  " Setting SIGN_DAC = 1 and DACB.";
              settings[i_roc][page]["TRIM_INV"] = 0;
              settings[i_roc][page]["SIGN_DAC"] = 1;
              settings[i_roc][page]["DACB"] = val;
            }
          }
        } else /* ecal */ {
          double optim = 32 - (scale * 32);
          uint64_t val = static_cast<uint64_t>(optim);
          pflib_log(trace) << "Scale " << scale << " giving optimal value of "
                           << optim
                           << " for TRIM_INV"
                              " which rounds to "
                           << val;
          if (val == 0) {
            pflib_log(debug)
                << "Channel" << ch
                << " is above target but too close to use TRIM_INV "
                   "to lower, skipping.";
          } else {
            pflib_log(debug)
                << "Channel" << ch << " is above target, setting TRIM_INV.";
            settings[i_roc][page]["TRIM_INV"] = val;
          }
        }  // end if ecal
      }  // end if baseline > target
    }  // end loop over channels
  }  // end loop over rocs
  for (const auto& [i_roc, values] : ignored_channels) {
    pflib_log(info) << "ROC" << i_roc << " ignored channels: "
                    << SpaceSeparated(values.begin(), values.end());
  }
  return settings;
}

}  // namespace pflib::algorithm
