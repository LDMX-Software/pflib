#include "toa_vref_scan.h"

#include "../daq_run.h"
#include "../tasks/toa_vref_scan.h"
#include "get_toa_efficiencies.h"
#include "pflib/utility/efficiency.h"
#include "pflib/utility/string_format.h"

namespace pflib::algorithm {

// Templated helpder function
template <class EventPacket>
static void toa_vref_runs(Target* tgt, ROC& roc, size_t n_events) {
  static auto the_log_{::pflib::logging::get("toa_vref_scan")};
  DecodeAndBuffer<EventPacket> buffer{n_events};

  // loop over runs, from toa_vref = 0 to = 255
  for (int toa_vref{0}; toa_vref < 256; toa_vref++) {
    pflib_log(info) << "testing toa_vref = " << toa_vref;
    auto test_handle = roc.testParameters()
                           .add("REFERENCEVOLTAGE_0", "TOA_VREF", toa_vref)
                           .add("REFERENCEVOLTAGE_1", "TOA_VREF", toa_vref)
                           .apply();
    usleep(10);
    daq_run(tgt, "PEDESTAL", buffer, n_events, 100);
    pflib_log(trace) << "finished toa_vref = " << toa_vref
                     << ", getting efficiencies";
    auto efficiencies = get_toa_efficiencies(buffer.get_buffer());
    pflib_log(trace)
        << "got channel efficiencies, getting max efficiency per link";
    for (int i_link{0}; i_link < 2; i_link++) {
      auto start = efficiencies.begin() +
                   36 * i_link;  // start at 0 for link 0, 36 for link 1
      auto end = start + 36;
      final_effs[i_link][toa_vref] = *std::max_element(start, end);
    }
    pflib_log(trace) << "got link efficiencies";
  }
}

std::map<std::string, std::map<std::string, uint64_t>> toa_vref_scan(
    Target* tgt, ROC roc) {
  static auto the_log_{::pflib::logging::get("toa_vref_scan")};

  /// do a run of 100 samples per toa_vref to measure the TOA
  /// efficiency when looking at pedestal data

  static const std::size_t n_events = 100;

  tgt->setup_run(1, pftool::state.daq_format_mode, 1);

  std::array<int, 2>
      target;  // toa_vref is a global parameter (1 value per link)

  // there is probably a better way to do the next line
  std::array<std::array<double, 256>, 2> final_effs;

  if (pftool::state.daq_format_mode == Target::DaqFormat::SIMPLEROC) {
    toa_vref_runs<pflib::packing::SingleROCEventPacket>(tgt, roc, n_events);
  } else if (pftool::state.daq_format_mode ==
             Target::DaqFormat::ECOND_SW_HEADERS) {
    toa_vref_runs<pflib::packing::MultiSampleECONDEventPacket>(tgt, roc,
                                                               n_events);
  } else {
    pflib_log(warn) << "Unsupported DAQ format ("
                    << static_cast<int>(pftool::state.daq_format_mode)
                    << ") in level_pedestals. Skipping pedestal leveling...";
  }

  pflib_log(info) << "sample collections done, deducing settings";
  // get the max toa_vref with non-zero efficiency? Iterate through the array
  // from bottom up.
  for (int i_link{0}; i_link < 2; i_link++) {
    int highest_non_zero_eff = -1;  // just a placeholder in case it's not found
    for (int toa_vref = final_effs[0].size(); toa_vref >= 0; toa_vref--) {
      if (final_effs[i_link][toa_vref] > 0.0) {
        highest_non_zero_eff =
            toa_vref + 10;  // need to add 10 since we don't want to overlap
                            // with highest pedestals!
        break;              // should break from link 0 into link 1
      }
    }
    target[i_link] = highest_non_zero_eff;  // store value
  }

  std::map<std::string, std::map<std::string, uint64_t>> settings;
  for (int i_link{0}; i_link < 2; i_link++) {
    std::string page{
        pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link)};
    settings[page]["TOA_VREF"] = target[i_link];
  }

  return settings;
}

}  // namespace pflib::algorithm
