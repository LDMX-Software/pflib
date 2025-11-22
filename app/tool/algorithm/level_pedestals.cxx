#include "level_pedestals.h"

#include "../daq_run.h"
#include "../tasks/level_pedestals.h"
#include "pflib/utility/median.h"
#include "pflib/utility/string_format.h"

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

namespace pflib::algorithm {

// Helper function to pull the 3 runs
template <class EventPacket>  // any use of <EventPacket> is a placeholder for
                              // what the function gets called with.
static void pedestal_runs(Target* tgt, ROC& roc, std::array<int, 72>& baseline,
                          std::array<int, 72>& highend,
                          std::array<int, 72>& lowend,
                          std::array<int, 2>& target, size_t n_events) {
  DecodeAndBuffer<EventPacket> buffer{n_events};
  static auto the_log_{::pflib::logging::get("level_pedestals")};

  {  // baseline run scope
    pflib_log(info) << "100 event baseline run";
    auto test_handle = roc.testParameters()
                           .add_all_channels("SIGN_DAC", 0)
                           .add_all_channels("DACB", 0)
                           .add_all_channels("TRIM_INV", 0)
                           .apply();
    daq_run(tgt, "PEDESTAL", buffer, n_events, 100);
    pflib_log(trace) << "baseline run done, getting channel medians";
    auto medians = get_adc_medians<EventPacket>(buffer.get_buffer());
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
    auto test_handle = roc.testParameters()
                           .add_all_channels("SIGN_DAC", 0)
                           .add_all_channels("DACB", 0)
                           .add_all_channels("TRIM_INV", 63)
                           .apply();
    daq_run(tgt, "PEDESTAL", buffer, n_events, 100);
    highend = get_adc_medians<EventPacket>(buffer.get_buffer());
  }

  {  // lowend run
    pflib_log(info) << "100 event lowend run";
    auto test_handle = roc.testParameters()
                           .add_all_channels("SIGN_DAC", 1)
                           .add_all_channels("DACB", 31)
                           .add_all_channels("TRIM_INV", 0)
                           .apply();
    daq_run(tgt, "PEDESTAL", buffer, n_events, 100);
    lowend = get_adc_medians<EventPacket>(buffer.get_buffer());
  }
}

template <class EventPacket>
static int get_adc(const EventPacket& p, int ch) {
  if constexpr (std::is_same_v<EventPacket,
                               pflib::packing::SingleROCEventPacket>) {
    return p.channel(ch).adc();
  } else if constexpr (std::is_same_v<
                           EventPacket,
                           pflib::packing::MultiSampleECONDEventPacket>) {
    // Use link specific channel calculation, this is done in
    // singleROCEventPacket.cxx for the other case
    //  // Use the "Sample Of Interest" inside the EventPacket (See MultiSample
    //  cxx file)
    int i_link = ch / 36;  // 0 or 1
    int i_ch = ch % 36;    // 0–35

    // ECONDEventPacket.h defines channel differently to SingleROCEventPacket.h
    return p.samples[p.i_soi].channel(i_link, i_ch).adc();
  } else {
    static_assert(sizeof(EventPacket) == 0,
                  "Unsupported packet type in get_adc()");
  }
}

template <class EventPacket>
static std::array<int, 72> get_adc_medians(
    const std::vector<EventPacket>& data) {
  std::array<int, 72> medians;
  /// reserve a vector of the appropriate size to avoid repeating allocation
  /// time for all 72 channels
  std::vector<int> adcs(data.size());
  for (int ch{0}; ch < 72; ch++) {
    for (std::size_t i{0}; i < adcs.size(); i++) {
      // adcs[i] = data[i].channel(ch).adc();
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

  // tgt->setup_run(1, Target::DaqFormat::SIMPLEROC, 1);
  // Use the DAQ format selected in the pftool DAQ->FORMAT menu so the
  // format mode can be chosen interactively by the user.
  tgt->setup_run(1, pftool::state.daq_format_mode, 1);
  pflib_log(info) << "Using DAQ format mode: "
                  << static_cast<int>(pftool::state.daq_format_mode);

  std::array<int, 2> target;
  std::array<int, 72> baseline, highend, lowend;

  // DecodeAndBuffer buffer{n_events};
  if (pftool::state.daq_format_mode == Target::DaqFormat::SIMPLEROC) {
    pedestal_runs<pflib::packing::SingleROCEventPacket>(
        tgt, roc, baseline, highend, lowend, target, n_events);

  } else if (pftool::state.daq_format_mode ==
             Target::DaqFormat::ECOND_SW_HEADERS) {
    pedestal_runs<pflib::packing::MultiSampleECONDEventPacket>(
        tgt, roc, baseline, highend, lowend, target, n_events);

  } else {
    pflib_log(warn) << "Unsupported DAQ format ("
                    << static_cast<int>(pftool::state.daq_format_mode)
                    << ") in level_pedestals. Skipping pedestal leveling...";
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
