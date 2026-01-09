#include "get_lpgbt_temps.h"

#include "pflib/OptoLink.h"

struct CalibConstants {
  int vref_tune = 0;
  double tj_user = 0.0;

  // automatically filled from .csv
  std::map<std::string, std::string> values;

  std::string get(const std::string& key) const {
    auto it = values.find(key);
    if (it != values.end()) return it->second;
    printf("WARNING: missing calibration constant '%s'\n", key.c_str());
    return "0.0";
  }
};

static void get_calib_constants(std::string& chipid, std::string csv_file,
                                std::string yaml_file, std::string lpgbt_name) {
  CalibConstants calib;

  printf(" --- Fetching calibration constants from .csv for %s ---\n",
         chipid.c_str());

  std::ifstream file(csv_file);
  if (!file.is_open()) {
    printf("  > ERROR: Could not open CSV.\n");
  } else {
    std::string line;
    // Skip first three lines
    for (int i = 0; i < 3; i++) std::getline(file, line);

    std::getline(file, line);
    auto header = split_csv(line);

    // Build name → index map
    std::unordered_map<std::string, int> col;
    for (int i = 0; i < header.size(); i++) col[header[i]] = i;

    // Search row by row for  CHIPID

    bool found = false;
    while (std::getline(file, line)) {
      auto row = split_csv(line);
      if (row[col["CHIPID"]] == chipid) {
        printf(" --- Found Calibration Row for CHIPID %s ---\n",
               chipid.c_str());
        for (size_t i = 0; i < row.size(); i++) {
          const std::string& colname = header[i];
          const std::string& raw = row[i];
          try {
            calib.values[colname] = raw;
          } catch (...) {
            printf("WARNING: Could not parse '%s' for column '%s'\n",
                   raw.c_str(), colname.c_str());
          }
        }
        found = true;
        break;
      }
    }
    if (!found) {
      printf("  > ERROR: CHIPID '%s' not found in calibration CSV.\n",
             chipid.c_str());
      return;
    }
  }

  printf(" --- Saving Calibration Results ---\n");
  YAML::Node results;

  std::ifstream fin(yaml_file);

  if (fin.good()) {
    try {
      results = YAML::Load(fin);
    } catch (...) {
      printf("  > WARNING: Failed to parse existing YAML. Starting fresh.\n");
    }
  }
  fin.close();

  YAML::Node entry;
  YAML::Node cal;

  for (const auto& kv : calib.values) {
    const std::string& key = kv.first;
    std::string val = kv.second;
    cal[key] = val;
  }
  entry["card_type"] = lpgbt_name;
  entry["calib_constants"] = cal;
  results[chipid] = entry;

  std::ofstream fout(yaml_file);
  if (!fout.good()) {
    printf("  > ERROR: Could not write to YAML file '%s'\n", yaml_file.c_str());
    return;
  }
  fout << results;
  fout.close();
  printf("  > Successfully wrote calibration data to '%s'\n",
         yaml_file.c_str());
}

static void read_internal_temp(pflib::lpGBT lpgbt_card,
                               const std::string& chipid_hex,
                               const std::string& results_file) {
  YAML::Node results = YAML::LoadFile(results_file);
  YAML::Node entry = results[chipid_hex];
  YAML::Node cal = entry["calib_constants"];

  lpgbt_card.read_internal_temp_precise(cal);
}

void get_lpgbt_temps(Target* tgt) {
  std::string lpgbt_name = "DAQ";
  pflib::lpGBT lpgbt_card{tgt->get_opto_link(lpgbt_name).lpgbt_transport()};
  uint32_t chipid = lpgbt_card.read_efuse(0);
  printf(" --- Current lpGBT CHIPID: %08X ---\n", chipid);

  char chipid_str[9];
  snprintf(chipid_str, sizeof(chipid_str), "%08X", chipid);
  std::string chipid_hex = chipid_str;

  const std::string yaml_file = "calibration_results.yaml";
  const std::string csv_file = "lpgbt_calibration.csv";

  bool use_avgs = pftool::readline_bool("Use average constants?", true);

  if (use_avgs) {
    lpgbt_card.read_internal_temp_avg();
    return;
  } else {
    bool verify = pftool::readline_bool(
        "Using precise constants may require downloading and unzipping a large "
        "CSV file (330 MB) - Continue?",
        false);
    if (!verify) {
      printf("  > Continuing with average constants...\n");
      lpgbt_card.read_internal_temp_avg();
      return;
    }
  }

  bool have_yaml_for_chip = yaml_has_chipid(yaml_file, chipid_hex);

  if (have_yaml_for_chip) {
    printf("  > Found calibration constants...\n");
    read_internal_temp(lpgbt_card, chipid_hex, yaml_file);
    return;
  } else if (file_exists(csv_file)) {
    printf(
        "  > Did not find calibration file '%s', producing a file from "
        "'%s'...\n",
        yaml_file.c_str(), csv_file.c_str());
    get_calib_constants(chipid_hex, csv_file, yaml_file, lpgbt_name);
    read_internal_temp(lpgbt_card, chipid_hex, yaml_file);
    return;
  } else {
    printf(
        "  > Did not find calibration CSV file... Downloading from CERN...\n");
    std::string dl_cmd =
        "wget "
        "https://lpgbt.web.cern.ch/lpgbt/calibration/"
        "lpgbt_calibration_latest.zip";
    std::system(dl_cmd.c_str());
    std::string unzip_cmd = "unzip lpgbt_calibration_latest.zip";
    std::system(unzip_cmd.c_str());
    std::string rm_cmd =
        "rm -rf lpgbt_calibration_latest.zip lpgbt_calibration.db";
    std::system(rm_cmd.c_str());

    get_calib_constants(chipid_hex, csv_file, yaml_file, lpgbt_name);
    read_internal_temp(lpgbt_card, chipid_hex, yaml_file);
    return;
  }
}
