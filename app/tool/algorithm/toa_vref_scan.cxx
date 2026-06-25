#include "toa_vref_scan.h"

#include "../daq_run.h"
#include "../tasks/toa_vref_scan.h"
#include "get_toa_efficiencies.h"
#include "pflib/utility/efficiency.h"
#include "pflib/utility/string_format.h"

namespace pflib::algorithm {

std::map<int, std::map<std::string, std::map<std::string, uint64_t>>> toa_vref_scan(
    Target* tgt) {
  static auto the_log_{::pflib::logging::get("toa_vref_scan")};

  /// do a run of 100 samples per toa_vref to measure the TOA
  /// efficiency when looking at pedestal data

  static const std::size_t n_events = 100;

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  std::map<int, std::array<int, 2>> target;  // i_roc int for each variable
  std::map<int, std::array<std::array<double, 256>, 2>> final_effs;
  std::map<int, std::map<std::string, std::map<std::string, uint64_t>>> settings;

  // TODO 348
  DecodeAndBuffer buffer{n_events, tgt->nrocs() * 2};

  // loop over runs, from toa_vref = 0 to = 255
  for (int toa_vref{0}; toa_vref < 256; toa_vref++) {
    pflib_log(info) << "testing toa_vref = " << toa_vref;
    std::map<std::string, std::map<std::string, uint64_t>> parameters;
    parameters["REFERENCEVOLTAGE_0"]["TOA_VREF"] = toa_vref;
    parameters["REFERENCEVOLTAGE_1"]["TOA_VREF"] = toa_vref;
    auto test_params = tgt->tempApplyAllROCs(parameters);


    usleep(10);

    daq_run(tgt, "PEDESTAL", buffer, n_events, 100);

    pflib_log(trace) << "finished toa_vref = " << toa_vref
                     << ", getting efficiencies";

    for (int i_roc : tgt->roc_ids()) {
      // Debug callout for mapping
      const auto& mapping = tgt->getRocErxMapping();

      try {
        auto [first_erx, first_ch] = mapping.toErxChannel(i_roc, 0);
        auto [last_erx, last_ch] = mapping.toErxChannel(i_roc, 71);

        if (toa_vref == 0) {
          pflib_log(info) << "[DEBUG MAP] Mapping Check for ROC " << i_roc << ":"
                          << " Ch 0  -> eRx " << (int)first_erx << ", eCh " << (int)first_ch
                          << " | Ch 71  -> eRx " << (int)last_erx << ", eCh " << (int)last_ch;
        }
      }

      catch (const std::exception& e) {
        pflib_log(error) << "[DEBUG MAP] Critical: Mapping lookup threw an exception for ROC "
                         << i_roc << ". Message: " << e.what();
      }
      //

      auto efficiencies = get_toa_efficiencies(i_roc, mapping, buffer.get_buffer());
      pflib_log(trace) << "got channel efficiencies for ROC " << i_roc
                       << ", getting max efficiency per link";
      for (int i_link{0}; i_link < 2; i_link++){
        auto start = efficiencies.begin() + 36 * i_link;
        auto end = start + 36;

        double max_eff = *std::max_element(start, end);

        final_effs[i_roc][i_link][toa_vref] = max_eff;

        if (toa_vref % 32 == 0 || max_eff > 0.0) {
          pflib_log(trace) << "[DEBUG SCAN] VREF " << toa_vref
                           << " | ROC " << i_roc << " Link " << i_link
                           << " | Max Efficiency: " << max_eff;
        }
      }
      pflib_log(trace) << "got link efficiencies"; 
    }
  }
  pflib_log(info) << "sample collections done, deducing settings";
  // get the max toa_vref with non-zero efficiency? Iterate through the array
  // from bottom up.
  for (int i_roc : tgt->roc_ids()) {
    for (int i_link{0}; i_link < 2; i_link++) {
      int highest_non_zero_eff = -1;  // just a placeholder in case it's not found
      for (int toa_vref = final_effs[i_roc][i_link].size() - 1; toa_vref >= 0; toa_vref--) {
        if (toa_vref == (int)final_effs[i_roc][i_link].size() - 1) {
          pflib_log(trace) << "[DEBUG SEARCH] Starting backwards search for ROC " << i_roc
                           << " Link " << i_link << ". Initial val at max VREF: "
                           << final_effs[i_roc][i_link][toa_vref];
        }

        if (final_effs[i_roc][i_link][toa_vref] > 0.0) {
          highest_non_zero_eff =
              toa_vref + 10;  // need to add 10 since we don't want to overlap
                              // with highest pedestals!
          pflib_log(info) << "[DEBUG SEARCH] Success. Found threshold edge at VREF " << toa_vref
                          << " (Setting target to " << highest_non_zero_eff << ")";
          break;              // should break from link 0 into link 1
        }
      }

      if (highest_non_zero_eff < 0) {
        pflib_log(warn) << "ROC " << i_roc << " link " << i_link
                           << ": no non-zero TOA efficiency found, skipping";
        continue;
      }

      if (highest_non_zero_eff > 255) {
        pflib_log(warn) << "ROC " << i_roc << " link " << i_link
                           << ": deduced TOA_VREF " << highest_non_zero_eff 
                           << " out of range, clamping to 255";
        highest_non_zero_eff = 255;
      }

      target[i_roc][i_link] = highest_non_zero_eff;  // store value

    }
    // toa_vref is a global parameter (1 value per link)

    for (int i_link{0}; i_link < 2; i_link++) {
      std::string page{
          pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link)};
      settings[i_roc][page]["TOA_VREF"] = target[i_roc][i_link];
    }
  }
  return settings;
}


}  // namespace pflib::algorithm
