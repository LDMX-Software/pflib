#include "pflib/algorithm/toa_vref_scan.h"

#include "pflib/utility/string_format.h"
#include "pflib/utility/efficiency.h"

#include "pflib/DecodeAndBuffer.h"

/**
 * calculate the highest TOA_VREF value for each link, for which there is a non-zero TOA efficiency
 */
static std::array<double, 72> get_toa_efficiencies(const std::vector<pflib::packing::SingleROCEventPacket> &data) {
  std::array<double, 72> efficiencies;
  /// reserve a vector of the appropriate size to avoid repeating allocation time for all 72 channels
  std::vector<int> toas(data.size());
  for (int ch{0}; ch < 72; ch++) {
    for (std::size_t i{0}; i < toas.size(); i++) {
      toas[i] = data[i].channel(ch).toa();
    }
    // might need something here if it freaks out when toas.size() == 0
    // BUT I CAN'T FIGURE OUT HOW TO MAKE THE LOG WORK, SO MAYBE SOMEBODY CAN FIX THAT?
    // if (toas.empty()) {
    //   pflib_log(warn) << "No TOA values found for channel " << ch
    //                   << ", setting efficiency to 0";
    //   efficiencies[ch] = 0.0;
    //   continue;
    // }
    efficiencies[ch] = pflib::utility::efficiency(toas);
  }
  return efficiencies;
}

namespace pflib::algorithm {

std::map<std::string, std::map<std::string, int>>
toa_vref_scan(Target* tgt, ROC roc) {
  static auto the_log_{::pflib::logging::get("toa_vref_scan")};

  /// do a run of 100 samples per toa_vref to measure the TOA
  /// efficiency when looking at pedestal data

  static const std::size_t n_events = 100;

  tgt->setup_run(1, Target::DaqFormat::SIMPLEROC, 1);

  std::array<int, 2> target; //toa_vref is a global parameter (1 value per link)
  std::array<double, 72> chan_effs;

  // there is probably a better way to do the next line
  std::array<std::array<double, 256>, 2> final_effs;
  DecodeAndBuffer buffer{n_events}; // working in buffer, not in writer

  // loop over runs, from toa_vref = 0 to = 255

  for (int toa_vref{0}; toa_vref < 256; toa_vref++) {
    pflib_log(info) << "testing toa_vref = " << toa_vref;
    auto test_handle = roc.testParameters()
      .add("REFERENCEVOLTAGE_0", "TOA_VREF", toa_vref)
      .add("REFERENCEVOLTAGE_1", "TOA_VREF", toa_vref)
      .apply();
    //TODO: Do we need next line? Put it there so that parameters set properly. Remove to save time?
    usleep(10);
    tgt->daq_run("PEDESTAL", buffer, n_events, 100);
    pflib_log(trace) << "finished toa_vref = " << toa_vref << ", getting efficiencies";
    auto efficiencies = get_toa_efficiencies(buffer.get_buffer());
    chan_effs = efficiencies;
    pflib_log(trace) << "got channel efficiencies, getting max efficiency per link";
    for (int i_link{0}; i_link < 2; i_link++) {
      auto start = efficiencies.begin() + 36 * i_link; // start at 0 for link 0, 36 for link 1
      auto end = start + 36;
      auto max_index = std::max_element(start, end);
      double max_eff = *max_index; // not sure how, but Google said this is how to do it
      final_effs[i_link][toa_vref] = max_eff; // store it
    }
    pflib_log(trace) << "got link efficiencies";
  }

  pflib_log(info) << "sample collections done, deducing settings";
  // get the max toa_vref with non-zero efficiency? Iterate through the array from bottom up.
  for (int i_link{0}; i_link < 2; i_link++) {
    int highest_non_zero_eff = -1; // just a placeholder in case it's not found
    for (int toa_vref = final_effs[0].size(); toa_vref >= 0; toa_vref--) {
      if (final_effs[i_link][toa_vref] > 0.0) {
        highest_non_zero_eff = toa_vref + 10; // need to add 10 since we don't want to overlap with highest pedestals!
        break; // should break from link 0 into link 1
      }
    }
    target[i_link] = highest_non_zero_eff; //store value
  }

  std::map<std::string, std::map<std::string, int>> settings;
  for (int i_link{0}; i_link < 2; i_link++) {
    std::string page{pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link)};
    settings[page]["TOA_VREF"] = target[i_link];
  }

  return settings;
}

}
