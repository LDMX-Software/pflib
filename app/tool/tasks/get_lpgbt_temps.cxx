#include "get_lpgbt_temps.h"

#include "pflib/OptoLink.h"

struct CalibConstants {
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

    // Build name -> index map
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

  // Now save out the results to the yaml file
  printf(" --- Saving Calibration Results ---\n");
  YAML::Node results;

  std::ifstream fin(yaml_file);

  if (fin.good()) {
    try {
      results = YAML::Load(fin);
    } catch (...) {
      printf("  > WARNING: Failed to parse existing YAML. Starting fresh...\n");
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

static std::string read_chipid(pflib::lpGBT& lpgbt_card) {
  uint32_t chipid = lpgbt_card.read_efuse(0);
  printf(" --- Current lpGBT CHIPID: %08X ---\n", chipid);
  char chipid_str[9];
  snprintf(chipid_str, sizeof(chipid_str), "%08X", chipid);
  return chipid_str;
}

// Load this chip's constants from the yaml, making it from the CERN csv
// (downloaded if missing) when needed
static bool load_calib(std::string& chipid_hex, const std::string& lpgbt_name,
                       YAML::Node& cal) {
  const std::string yaml_file = "calibration_results.yaml";
  const std::string csv_file = "lpgbt_calibration.csv";

  if (yaml_has_chipid(yaml_file, chipid_hex)) {
    printf("  > Found calibration constants...\n");
  } else if (file_exists(csv_file)) {
    printf(
        "  > Did not find calibration file '%s', producing a file from "
        "'%s'...\n",
        yaml_file.c_str(), csv_file.c_str());
    get_calib_constants(chipid_hex, csv_file, yaml_file, lpgbt_name);
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
  }

  if (!yaml_has_chipid(yaml_file, chipid_hex)) {
    printf("  > ERROR: no calibration constants for CHIPID %s\n",
           chipid_hex.c_str());
    return false;
  }
  cal = YAML::LoadFile(yaml_file)[chipid_hex]["calib_constants"];
  return true;
}

void get_lpgbt_temps(Target* tgt) {
  std::string lpgbt_name = "DAQ";
  pflib::lpGBT lpgbt_card{tgt->get_opto_link(lpgbt_name).lpgbt_transport()};
  std::string chipid_hex = read_chipid(lpgbt_card);

  // If the user wants to use the average values
  bool use_avgs = pftool::readline_bool("Use average constants?", true);
  if (use_avgs) {
    lpgbt_card.read_internal_temp_avg();
    return;
  } else {
    bool verify = pftool::readline_bool(
        "Using precise constants may require downloading and unzipping a large "
        "CSV file (330 MB) - Continue?",
        true);
    if (!verify) {
      printf("  > Continuing with average constants...\n");
      lpgbt_card.read_internal_temp_avg();
      return;
    }
  }

  // If the user wants precise calculations
  YAML::Node cal;
  if (!load_calib(chipid_hex, lpgbt_name, cal)) return;
  lpgbt_card.read_internal_temp_precise(cal);
}

void read_rtd(Target* tgt) {
  std::string lpgbt_name = pftool::readline("lpGBT", {"DAQ", "TRG"}, "DAQ");
  // ADC5 on the ECal SMM DAQ lpGBT
  int pin = pftool::readline_int("ADC pin with the RTD", 5);
  int r0 = pftool::readline_int("RTD resistance at 0 C (ohm)", 1000);
  if (pin < 0 || pin > 7) {
    printf("  > ERROR: RTD pin must be ADC0-7\n");
    return;
  }
  pflib::lpGBT lpgbt_card{tgt->get_opto_link(lpgbt_name).lpgbt_transport()};
  std::string chipid_hex = read_chipid(lpgbt_card);

  // per-chip constants, needs the CERN csv (330 MB)
  YAML::Node cal;
  if (!load_calib(chipid_hex, lpgbt_name, cal)) return;
  auto c = [&](const std::string& key) { return cal[key].as<double>(); };
  std::string cdac = "CDAC" + std::to_string(pin) + "_";

  // tune VREF for the junction temperature, as in read_internal_temp_precise
  int vref_tune = lpgbt_card.find_vref_tune(c("VREF_SLOPE"), c("VREF_OFFSET"),
                                            c("TEMPERATURE_UNCALVREF_SLOPE"),
                                            c("TEMPERATURE_UNCALVREF_OFFSET"));
  double tj = (vref_tune - c("VREF_OFFSET")) / c("VREF_SLOPE");

  auto volts = [&](int code) {
    const int samples = 10;
    double sum = 0;
    for (int i = 0; i < samples; i++)
      sum += lpgbt_card.adc_resistance_read(pin, code);
    double adc = sum / samples;
    return adc * (c("ADC_X2_SLOPE") + tj * c("ADC_X2_SLOPE_TEMP")) +
           c("ADC_X2_OFFSET") + tj * c("ADC_X2_OFFSET_TEMP");
  };

  // raise the current until the pin is near mid range, where the ADC is best
  int code = 8;
  double v = volts(code);
  while (v < 0.5 && code < 255) {
    code = std::min(code + 8, 255);
    v = volts(code);
  }
  if (v > 0.95) {
    printf("  > ERROR: ADC%d at %.3f V with %d current steps, RTD open?\n", pin,
           v, code);
    return;
  }
  if (v < 0.05) {
    printf("  > ERROR: ADC%d at %.3f V at full current, RTD shorted?\n", pin,
           v);
    return;
  }

  // ideal current, minus the leak through the source output resistance R0/code
  double amps = (code - (c(cdac + "OFFSET") + tj * c(cdac + "OFFSET_TEMP"))) /
                (c(cdac + "SLOPE") + tj * c(cdac + "SLOPE_TEMP"));
  double r0_step = c(cdac + "R0") + tj * c(cdac + "R0_TEMP");
  double ohms = v / (amps - v * code / r0_step);
  double temperature = rtd_resistance_to_celsius(ohms, r0);

  printf("  > lpGBT junction temperature: %.1f C (VREFTUNE %d)\n", tj,
         vref_tune);
  printf("  > CURDAC %d = %.1f uA, ADC%d = %.4f V, R = %.1f ohm\n", code,
         amps * 1e6, pin, v, ohms);
  printf("  > ----------------------------------------------------\n");
  printf("  > RTD Temperature: %.2f degrees C\n", temperature);
  printf("  > ----------------------------------------------------\n");
}
