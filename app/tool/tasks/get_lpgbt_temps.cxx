#include "get_lpgbt_temps.h"

#include "pflib/OptoLink.h"

struct CalibConstants {
  int vref_tune = 0;
  double tj_user = 0.0;

  // automatically filled from .csv
  std::unordered_map<std::string, std::string> values;

  std::string get(const std::string& key) const {
    auto it = values.find(key);
    if (it != values.end()) return it->second;
    printf("WARNING: missing calibration constant '%s'\n", key.c_str());
    return "0.0";
  }
};

uint8_t REG_VREFTUNE = 0x01d;
uint8_t REG_CURDACVALUE = 0x06c;
uint16_t REG_ADCMON = 0x122;

void save_results(const std::string& results_file,
                  const std::string& lpgbt_name, const uint32_t chipid,
                  int optimal_vref_tune, const CalibConstants& calib) {
  printf(" --- Saving Calibration Results ---\n");

  YAML::Node results;

  // Load existing YAML file if present
  std::ifstream fin(results_file);
  if (fin.good()) {
    try {
      results = YAML::Load(fin);
    } catch (...) {
      printf("  > WARNING: Failed to parse existing YAML. Starting fresh.\n");
    }
  }
  fin.close();

  char buf[9];
  snprintf(buf, sizeof(buf), "%08X", chipid);
  std::string chipid_hex = buf;

  // Insert / update entry
  YAML::Node entry;
  entry["chipid"] = chipid_hex;
  entry["vref_tune"] = optimal_vref_tune;
  entry["tj_user"] = calib.tj_user;

  YAML::Node cal;

  for (const auto& kv : calib.values) {
    const std::string& key = kv.first;
    std::string val = kv.second;
    cal[key] = val;
  }
  entry["calib_constants"] = cal;

  results[lpgbt_name] = entry;

  // Write YAML back
  std::ofstream fout(results_file);
  if (!fout.good()) {
    printf("  > ERROR: Could not write YAML file '%s'\n", results_file.c_str());
    return;
  }

  fout << results;
  fout.close();

  printf("  > Successfully wrote calibration data to '%s'\n",
         results_file.c_str());
}

static std::vector<uint16_t> read_adc_at_current(
    Target* tgt, int adc_channel, int n_pin, int curdacvalue,
    const std::string& results_file, int samples = 20) {
  pflib::lpGBT lpgbt_daq{tgt->get_opto_link("DAQ").lpgbt_transport()};

  YAML::Node file_results = YAML::LoadFile(results_file);
  YAML::Node entry = file_results["DAQ"];
  int vref_tune = entry["vref_tune"].as<int>();

  std::vector<uint16_t> results;
  results.reserve(samples);

  lpgbt_daq.write(REG_CURDACVALUE, curdacvalue);
  lpgbt_daq.write(REG_VREFTUNE, vref_tune);
  uint8_t vreftune = lpgbt_daq.read(REG_VREFTUNE);
  printf("  > VREFTUNE REG: %d\n", vreftune);

  uint8_t curdac_value = lpgbt_daq.read(REG_CURDACVALUE);

  for (int i = 0; i < samples; ++i) {
    if (curdacvalue == 0) {
      // Internal Temp
      uint16_t adc_value = lpgbt_daq.adc_read(14, 15, 0);
      results.push_back(adc_value);

    } else {
      // ADC Temp
      uint16_t adc_value =
          lpgbt_daq.adc_resistance_read(adc_channel, curdacvalue, 0);
      results.push_back(adc_value);
      // printf("     Sample %02d : %d\n", i, adc_value);
    }
  }

  return results;
}

double calculate_tj_user(const std::string& results_file,
                         const std::string& lpgbt_name) {
  YAML::Node results = YAML::LoadFile(results_file);

  if (!results[lpgbt_name]) {
    std::cerr << "ERROR: No entry found for '" << lpgbt_name
              << "' in results file.\n";
    return -1;
  }

  YAML::Node entry = results[lpgbt_name];

  // Extract vref_tune
  if (!entry["vref_tune"]) {
    std::cerr << "ERROR: vref_tune missing in YAML entry.\n";
    return -1;
  }
  int vref_tune = entry["vref_tune"].as<int>();

  // Extract calibration constants
  if (!entry["calib_constants"]) {
    std::cerr << "ERROR: calib_constants missing in YAML entry.\n";
    return -1;
  }

  YAML::Node cal = entry["calib_constants"];

  double vref_slope = cal["VREF_SLOPE"].as<double>();
  double vref_offset = cal["VREF_OFFSET"].as<double>();

  double tj_user = (vref_tune - vref_offset) / vref_slope;

  printf(
      "  > Inferred Junction Temperature (TJ_USER) from vref_tune: %f degrees "
      "C\n",
      tj_user);
  entry["tj_user"] = tj_user;
  results[lpgbt_name] = entry;

  std::ofstream fout(results_file);
  fout << results;
  fout.close();

  printf("  > Updated '%s' with TJ_USER\n", results_file.c_str());

  return tj_user;
}

static void get_calib_constants(Target* tgt) {
  pflib::lpGBT lpgbt_daq{tgt->get_opto_link("DAQ").lpgbt_transport()};

  CalibConstants c;
  const std::string& lpgbt_name = "DAQ";

  printf(" --- Finding CHIPID ---\n");
  uint32_t chipid = lpgbt_daq.read_efuse(0);
  printf(" CHIPID: %08X\n", chipid);

  char chipid_str[9];
  snprintf(chipid_str, sizeof(chipid_str), "%08X", chipid);
  std::string chipid_hex = chipid_str;

  printf(" --- Fetching calibration constants from .csv for %08X ---\n",
         chipid);
  std::string csv_filename = "lpgbt_calibration.csv";

  if (!file_exists(csv_filename)) {
    printf("  > Local file '%s' not found...\n", csv_filename.c_str());
  } else {
    printf("  > Found local calibration file: '%s'\n", csv_filename.c_str());
  }

  printf("  > Parsing CSV data...\n");

  std::ifstream file(csv_filename);
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
      if (row[col["CHIPID"]] == chipid_hex) {
        printf("  -- Loaded Calibration Row for CHIPID %s --\n",
               chipid_hex.c_str());
        for (size_t i = 0; i < row.size(); i++) {
          const std::string& colname = header[i];
          const std::string& raw = row[i];
          try {
            c.values[colname] = raw;
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
      printf("  > ERROR: CHIPID '%s' not found in calibration DB.\n",
             chipid_hex.c_str());
      return;
    }
  }

  printf("  > Success! Loaded constants for %08X\n", chipid);

  printf(" --- Performing Live VREFTUNE Calibration ---\n");
  printf("  > Taking uncalibrated reading with VREFTUNE = 0...\n");

  lpgbt_daq.write(REG_VREFTUNE, 0);
  uint8_t vreftune = lpgbt_daq.read(REG_VREFTUNE);
  printf("  > VREFTUNE REG: %d\n", vreftune);

  save_results("calibration_results.yaml", lpgbt_name, chipid, 0, c);

  std::vector<uint16_t> adc_vals =
      read_adc_at_current(tgt, 14, 15, 0, "calibration_results.yaml");

  double adc_sum = 0;
  for (auto v : adc_vals) adc_sum += v;
  double adc_avg = adc_sum / adc_vals.size();

  printf("  > ADC Average: %f\n", adc_avg);

  float tj_user = adc_avg * to_double(c.get("TEMPERATURE_UNCALVREF_SLOPE")) +
                  to_double(c.get("TEMPERATURE_UNCALVREF_OFFSET"));
  printf("  > Esitimated junction temperature (TJ_USER): %f degrees C\n",
         tj_user);

  int optimal_vref_tune =
      static_cast<int>(std::round(tj_user * to_double(c.get("VREF_SLOPE")) +
                                  to_double(c.get("VREF_OFFSET"))));
  printf("  > Optimal VREFTUNE: %d\n", optimal_vref_tune);

  save_results("calibration_results.yaml", lpgbt_name, chipid,
               optimal_vref_tune, c);
}

static void read_internal_temp(Target* tgt, const std::string& results_file) {
  pflib::lpGBT lpgbt_daq{tgt->get_opto_link("DAQ").lpgbt_transport()};
  double tj_user = calculate_tj_user("calibration_results.yaml", "DAQ");
  YAML::Node results = YAML::LoadFile(results_file);
  YAML::Node entry = results["DAQ"];
  YAML::Node cal = entry["calib_constants"];

  double adc_slope = cal["ADC_X2_SLOPE"].as<double>();
  double adc_slope_temp = cal["ADC_X2_SLOPE_TEMP"].as<double>();
  double adc_offset = cal["ADC_X2_OFFSET"].as<double>();
  double adc_offset_temp = cal["ADC_X2_OFFSET_TEMP"].as<double>();
  double temp_slope = cal["TEMPERATURE_SLOPE"].as<double>();
  double temp_offset = cal["TEMPERATURE_OFFSET"].as<double>();

  // Reset temperature sensor
  lpgbt_daq.write(REG_ADCMON, uint8_t(1) << 5);
  usleep(1000);
  lpgbt_daq.write(REG_ADCMON, uint8_t(0) << 5);

  std::vector<uint16_t> adc_vals =
      read_adc_at_current(tgt, 14, 15, 0, "calibration_results.yaml");

  double adc_sum = 0;
  for (auto v : adc_vals) adc_sum += v;
  double adc_avg = adc_sum / adc_vals.size();

  double vadc_v = adc_avg * (adc_slope + tj_user * adc_slope_temp) +
                  adc_offset + tj_user * adc_offset_temp;

  printf("  > Calibrated Voltage: %.4f V\n", vadc_v);
  double temperature_c = (vadc_v * temp_slope) + temp_offset;
  printf("  > ----------------------------------------------------\n");
  printf("  > lpGBT Internal Temperature: %.2f degrees C\n", temperature_c);
  printf("  > ----------------------------------------------------\n");
}

void get_lpgbt_temps(Target* tgt) {
  pflib::lpGBT lpgbt_daq{tgt->get_opto_link("DAQ").lpgbt_transport()};
  uint32_t chipid = lpgbt_daq.read_efuse(0);
  printf(" --- Current lpGBT CHIPID: %08X ---\n", chipid);

  const std::string yaml_file = "calibration_results.yaml";
  const std::string csv_file = "lpgbt_calibration.csv";

  if (file_exists(yaml_file)) {
    printf("  > Found existing calibration file '%s'...\n", yaml_file.c_str());
    bool have_yaml_for_chip = yaml_has_chipid(yaml_file, "DAQ", chipid);
    if (!have_yaml_for_chip) {
      printf(
          "  > Calibration constants for this CHIPID not found in present "
          "YAML.\n");
      if (!file_exists(csv_file)) {
        printf(
            "  > No calibration CSV file found... Downloading from CERN...\n");
        std::string dl_cmd =
            "wget "
            "https://lpgbt.web.cern.ch/lpgbt/calibration/"
            "lpgbt_calibration_latest.zip";
        std::system(dl_cmd.c_str());

        printf("  > File downloaded... Unzipping...\n");
        std::string unzip_cmd = "unzip lpgbt_calibration_latest.zip";
        std::system(unzip_cmd.c_str());

        get_calib_constants(tgt);
        read_internal_temp(tgt, "calibration_results.yaml");

        printf("  > Removing .zip and .db files...\n");
        std::string rm_cmd =
            "rm -rf lpgbt_calibration_latest.zip lpgbt_calibration.db";
        std::system(rm_cmd.c_str());
      } else {
        get_calib_constants(tgt);
        read_internal_temp(tgt, "calibration_results.yaml");
      }
    } else {
      read_internal_temp(tgt, "calibration_results.yaml");
    }
  } else {
    printf("  > YAML file doesn't exist... turning to CSV...\n");
    if (!file_exists(csv_file)) {
      printf("  > No calibration CSV file found... Downloading from CERN...\n");
      std::string dl_cmd =
          "wget "
          "https://lpgbt.web.cern.ch/lpgbt/calibration/"
          "lpgbt_calibration_latest.zip";
      std::system(dl_cmd.c_str());

      printf("  > File downloaded... Unzipping...\n");
      std::string unzip_cmd = "unzip lpgbt_calibration_latest.zip";
      std::system(unzip_cmd.c_str());

      printf("  > Removing .zip and .db files...\n");
      std::string rm_cmd =
          "rm -rf lpgbt_calibration_latest.zip lpgbt_calibration.db";
      std::system(rm_cmd.c_str());
    }
    get_calib_constants(tgt);
    read_internal_temp(tgt, "calibration_results.yaml");
  }
}
