#include "level_pedestals_inv_vref.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

#include "../algorithm/level_pedestals.h"
#include "../algorithm/inv_vref_align.h"

#include "inv_vref_scan.h"

#include <nlohmann/json.hpp>

#include "../daq_run.h"
#include "../econ_links.h"

static void settings_to_yaml(
    YAML::Emitter& out,
    const std::map<std::string, std::map<std::string, uint64_t>>& settings) {

  out << YAML::BeginMap;
  for (const auto& page : settings) {
    out << YAML::Key << page.first;
    out << YAML::Value << YAML::BeginMap;
    for (const auto& param : page.second) {
      out << YAML::Key << param.first << YAML::Value << param.second;
    }
    out << YAML::EndMap;
  }
  out << YAML::EndMap;
}

void inv_vref_scan(Target* tgt) {
  int nevents = 1;
  int inv_vref = 0;

  std::string output_filepath = "inv_vref_scan_level.csv";
  int ch_link0 = 17;
  int ch_link1 = 51;
  std::array<int, 2> channels = {ch_link0, ch_link1};

  auto roc = tgt->roc(pftool::state.iroc);

  if (pftool::state.daq_format_mode == Target::DaqFormat::SIMPLEROC) {
    inv_vref_scan_writer<pflib::packing::SingleROCEventPacket>(
        tgt, roc, nevents, output_filepath, channels, inv_vref);
  } else if (pftool::state.daq_format_mode ==
             Target::DaqFormat::ECOND_SW_HEADERS) {
    inv_vref_scan_writer<pflib::packing::MultiSampleECONDEventPacket>(
        tgt, roc, nevents, output_filepath, channels, inv_vref);
  }
}

template <class EventPacket>
static void inv_vref_scan_writer(Target* tgt, pflib::ROC& roc, size_t nevents,
                                 std::string& output_filepath,
                                 std::array<int, 2>& channels, int& inv_vref) {
  int link = 0;
  int i_ch = 0;  // 0–35
  int n_links = 2;
  if constexpr (std::is_same_v<EventPacket,
                               pflib::packing::MultiSampleECONDEventPacket>) {
    n_links = determine_n_links(tgt);
  }
  DecodeAndWriteToCSV<EventPacket> writer{
      output_filepath,
      [&](std::ofstream& f) {
        nlohmann::json header;
        header["scan_type"] = "CH_#.INV_VREF sweep";
        header["trigger"] = "PEDESTAL";
        header["nevents_per_point"] = nevents;
        f << "# " << header << "\n"
          << "INV_VREF";
        for (int ch : channels) {
          f << ',' << ch;
        }
        f << '\n';
      },
      [&](std::ofstream& f, const EventPacket& ep) {
        f << inv_vref;
        for (int ch : channels) {
          link = (ch / 36);
          i_ch = ch % 36;
          if constexpr (std::is_same_v<EventPacket,
                                       pflib::packing::SingleROCEventPacket>) {
            f << ',' << ep.channel(ch).adc();
          } else if constexpr (
              std::is_same_v<EventPacket,
                             pflib::packing::MultiSampleECONDEventPacket>) {
            f << ',' << ep.samples[ep.i_soi].channel(link, i_ch).adc();
          }
        }
        f << '\n';
      },
      n_links};

  tgt->setup_run(1 /* dummy - not stored */, pftool::state.daq_format_mode,
                 1 /* dummy */);

  // increment inv_vref in increments of 20. 10 bit value but only scanning to
  // 600
  for (inv_vref = 0; inv_vref <= 600; inv_vref += 20) {
    pflib_log(info) << "Running INV_VREF = " << inv_vref;
    // set inv_vref simultaneously for both links
    auto test_param = roc.testParameters()
                          .add("REFERENCEVOLTAGE_0", "INV_VREF", inv_vref)
                          .add("REFERENCEVOLTAGE_1", "INV_VREF", inv_vref)
                          .apply();
    // store current scan state in header for writer access
    daq_run(tgt, "PEDESTAL", writer, nevents, pftool::state.daq_rate);
  }
}
#include <numeric>
#include <cmath>
#include <algorithm>

// Helper to calculate optimal VREF from the scan data
static std::map<std::string, std::map<std::string, uint64_t>> calculate_vref_alignment(
    const std::string& csv_path, int target_adc) {
    
    std::map<std::string, std::map<std::string, uint64_t>> results;
    
    // 1. Read the CSV (Simplified parsing for the specific format in inv_vref_scan_writer)
    std::ifstream file(csv_path);
    std::string line, header;
    std::getline(file, line); // Skip JSON comment line
    std::getline(file, header); // Read CSV headers (INV_VREF,ch1,ch2...)

    std::stringstream ss(header);
    std::vector<std::string> col_names;
    std::string col;
    while (std::getline(ss, col, ',')) col_names.push_back(col);

    // Data containers: Key is channel string, Value is vector of ADC counts
    std::map<std::string, std::vector<double>> data;
    std::vector<double> vref_axis;

    while (std::getline(file, line)) {
        std::stringstream ss_row(line);
        std::string val;
        for (size_t i = 0; i < col_names.size(); ++i) {
            if (!std::getline(ss_row, val, ',')) break;
            if (i == 0) vref_axis.push_back(std::stod(val));
            else data[col_names[i]].push_back(std::stod(val));
        }
    }

    // 2. Perform Linear Regression for each channel
    for (auto const& [ch_str, adc_values] : data) {
        if (ch_str == "INV_VREF") continue;
        auto [min_it, max_it] = std::minmax_element(adc_values.begin(), adc_values.end());
        double min_val = *min_it;
        double max_val = *max_it;
        double lower = 0.1 * (max_val - min_val) + min_val;
        double upper = 0.8 * (max_val - min_val) + min_val;

        std::vector<double> x_reg, y_reg;
        for (size_t i = 0; i < adc_values.size(); ++i) {
            if (adc_values[i] >= lower && adc_values[i] <= upper) {
                x_reg.push_back(vref_axis[i]);
                y_reg.push_back(adc_values[i]);
            }
        }

        if (x_reg.size() < 2) continue;

        // Linear Regression: y = mx + c
        double n = x_reg.size();
        double sX = std::accumulate(x_reg.begin(), x_reg.end(), 0.0);
        double sY = std::accumulate(y_reg.begin(), y_reg.end(), 0.0);
        double sXX = std::inner_product(x_reg.begin(), x_reg.end(), x_reg.begin(), 0.0);
        double sXY = std::inner_product(x_reg.begin(), x_reg.end(), y_reg.begin(), 0.0);

        double slope = (n * sXY - sX * sY) / (n * sXX - sX * sX);
        double intercept = (sY - slope * sX) / n;

        // Solve for target: vref = (target - intercept) / slope
        uint64_t opt_vref = std::round((target_adc - intercept) / slope);
        
        int ch = std::stoi(ch_str);
        std::string page = (ch <= 35) ? "REFERENCEVOLTAGE_0" : "REFERENCEVOLTAGE_1";
        results[page]["INV_VREF"] = opt_vref;
    }

    return results;
}
void level_pedestals_inv_vref(Target* tgt) {
  auto roc = tgt->roc(pftool::state.iroc);

  // level pedestals in each link
  auto ped_settings = pflib::algorithm::level_pedestals(tgt, roc);
  roc.applyParameters(ped_settings);

  // run inv_vref_scan
  inv_vref_scan(Target* tgt);

  // algorithm to determine optimal inv_vref levels
  std::string scan_file = "inv_vref_scan_level.csv";
  int target_adc = 150; // Matching your Python default
  
  auto vref_settings = calculate_vref_alignment(scan_file, target_adc);

  // Apply these new VREF settings to the ROC before saving
  roc.applyParameters(vref_settings);
  

  // 4) Merge for YAML output
  // Start with pedestal settings, then overlay inv_vref pages
  auto combined = ped_settings;
  for (const auto& page : vref_settings) {
    combined[page.first] = page.second;
  }

  // If you truly want only INV_VREF/TRIM_INV/DACB, you can filter here.
  // BUT: DACB sometimes requires SIGN_DAC=1 to be meaningful when reapplying YAML.
  // I recommend keeping SIGN_DAC when present.
  //
  // If you want strict filtering, uncomment below:
  /*
  std::map<std::string, std::map<std::string, uint64_t>> filtered;
  for (const auto& page : combined) {
    for (const auto& kv : page.second) {
      if (kv.first == "INV_VREF" || kv.first == "TRIM_INV" || kv.first == "DACB" || kv.first == "SIGN_DAC") {
        filtered[page.first][kv.first] = kv.second;
      }
    }
  }
  combined = std::move(filtered);
  */

 YAML::Emitter out;
  settings_to_yaml(out, combined);

  // 5) Save YAML
  std::string fname = pftool::readline_path(
      "level-pedestals-inv-vref-roc-" + std::to_string(pftool::state.iroc) + "-settings",
      ".yaml");

  std::ofstream f{fname};
  if (!f.is_open()) {
    PFEXCEPTION_RAISE("File", "Unable to open file " + fname + ".");
  }
  f << out.c_str() << std::endl;

  // Optional: also print summary
  std::cout << "Applied pedestal leveling and INV_VREF alignment.\n"
            << "Wrote combined settings to " << fname << "\n";
}
