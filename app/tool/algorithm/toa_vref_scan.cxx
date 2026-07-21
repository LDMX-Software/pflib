#include "toa_vref_scan.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "../daq_run.h"
#include "../tasks/toa_vref_scan.h"
#include "get_toa_efficiencies.h"
#include "pflib/utility/efficiency.h"
#include "pflib/utility/string_format.h"
#include "trim_toa_scan.h"

namespace pflib::algorithm {

std::map<int, std::map<std::string, std::map<std::string, uint64_t>>>
toa_vref_scan(Target* tgt, bool scan_all, bool write_csv,
              const std::string& csv_filepath) {
  static auto the_log_{::pflib::logging::get("toa_vref_scan")};

  static const std::size_t n_events = 100;

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  std::map<int, std::array<int, 2>> target;  // i_roc int for each variable
  std::map<int, std::array<std::array<double, 256>, 2>> final_effs;
  std::map<int, std::map<std::string, std::map<std::string, uint64_t>>>
      settings;

  for (int i_roc : tgt->roc_ids()) {
    target[i_roc][0] = -1;
    target[i_roc][1] = -1;
  }

  size_t total_links_to_find = tgt->roc_ids().size() * 2;
  size_t links_found_count = 0;

  DecodeAndBuffer buffer{n_events, tgt->nrocs() * 2};

  // create a .csv file to save efficiency and vref data for analysis
  std::ofstream csv_file;

  if (write_csv) {
    std::string final_path = csv_filepath;

    if (final_path.empty()) {
      auto now = std::chrono::system_clock::now();
      auto in_time_t = std::chrono::system_clock::to_time_t(now);
      std::stringstream ss;
      ss << "toa_vref_scan_data_"
         << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S")
         << ".csv";
      final_path = ss.str();
    }

    csv_file.open(final_path);
    if (!csv_file) {
      pflib_log(error) << "Failed to open CSV file for writing: " << final_path;
      write_csv = false;
    } else {
      pflib_log(info) << "Saving scan data to file: " << final_path;
      csv_file << "TOA_VREF, ROC";
      for (int chan = 0; chan < 72; ++chan) {
        csv_file << "," << chan;
      }
      csv_file << "\n";
    }
  }

  // loop down from 255 to 0
  for (int toa_vref = 255; toa_vref >= 0; toa_vref--) {
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
      const auto& mapping = tgt->getRocErxMapping();

      auto efficiencies =
          get_toa_efficiencies(i_roc, mapping, buffer.get_buffer());
      pflib_log(trace) << "got channel efficiencies for ROC " << i_roc
                       << ", getting max efficiency per link";

      // Save raw channel efficiencies to .csv file
      if (write_csv) {
        csv_file << toa_vref << "," << i_roc;
        for (double eff : efficiencies) {
          csv_file << "," << eff;
        }
        csv_file << "\n";
      }

      for (int i_link{0}; i_link < 2; i_link++) {
        auto start = efficiencies.begin() + 36 * i_link;
        auto end = start + 36;

        double max_eff = *std::max_element(start, end);

        final_effs[i_roc][i_link][toa_vref] = max_eff;

        if (toa_vref % 32 == 0 || max_eff > 0.0) {
          pflib_log(trace) << "[DEBUG SCAN] VREF " << toa_vref << " | ROC "
                           << i_roc << " Link " << i_link
                           << " | Max Efficiency: " << max_eff;
        }

        // iterating from 255 down, find the first non-zero efficiency for each
        // link in each roc
        if (!scan_all) {
          if (target[i_roc][i_link] == -1 && max_eff > 0.0) {
            int calculated_vref = toa_vref + 10;
            target[i_roc][i_link] =
                (calculated_vref > 255) ? 255 : calculated_vref;
            links_found_count++;
            pflib_log(info)
                << "[DEBUG SEARCH] Success. Found threshold edge at VREF "
                << toa_vref << " (Setting target to " << target[i_roc][i_link]
                << ")";
          }
        }
      }
    }

    if (!scan_all && links_found_count == total_links_to_find) {
      pflib_log(info)
          << "All links found their threshold early. Breaking VREF loop at "
          << toa_vref;
      break;
    }
  }

  pflib_log(info) << "sample collections done, deducing settings";

  // get the max toa_vref with non-zero efficiency? Iterate through the array
  // from top down.

  if (scan_all) {
    for (int i_roc : tgt->roc_ids()) {
      for (int i_link{0}; i_link < 2; i_link++) {
        int highest_non_zero_eff =
            -1;  // just a placeholder in case it's not found
        for (int toa_vref = final_effs[i_roc][i_link].size() - 1; toa_vref >= 0;
             toa_vref--) {
          if (toa_vref == (int)final_effs[i_roc][i_link].size() - 1) {
            pflib_log(trace)
                << "[DEBUG SEARCH] Starting backwards search for ROC " << i_roc
                << " Link " << i_link << ". Initial val at max VREF: "
                << final_effs[i_roc][i_link][toa_vref];
          }

          if (final_effs[i_roc][i_link][toa_vref] > 0.0) {
            highest_non_zero_eff =
                toa_vref + 10;  // need to add 10 since we don't want to
                                // overlap with highest pedestals!
            pflib_log(info)
                << "[DEBUG SEARCH] Success. Found threshold edge at VREF "
                << toa_vref << " (Setting target to " << highest_non_zero_eff
                << ")";
            break;  // should break from link 0 into link 1
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
    }
  }

  for (int i_roc : tgt->roc_ids()) {
    for (int i_link{0}; i_link < 2; i_link++) {
      std::string page{
          pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link)};
      settings[i_roc][page]["TOA_VREF"] = target[i_roc][i_link];
    }
 }
  return settings;
}

}  // namespace pflib::algorithm
