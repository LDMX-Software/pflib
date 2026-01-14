#pragma once

#include <sys/stat.h>
#include <yaml-cpp/yaml.h>

#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>

#include "../pftool.h"
#include "pflib/zcu/lpGBT_ICEC_ZCU_Simple.h"

inline std::vector<std::string> split_csv(const std::string& line) {
  std::vector<std::string> out;
  std::stringstream ss(line);
  std::string item;

  while (std::getline(ss, item, ',')) {
    out.push_back(item);
  }
  return out;
}

inline bool file_exists(const std::string& name) {
  struct stat buffer;
  return (stat(name.c_str(), &buffer) == 0);
}

// Assumes a PT1000 sensor by default (r0=1000)
inline double rtd_resistance_to_celsius(double resistance_ohms,
                                        double r0 = 1000.0,
                                        double alpha = 0.00385) {
  // Not a physical value
  if (resistance_ohms < 0.0) return std::numeric_limits<double>::quiet_NaN();

  double temperature = (resistance_ohms / r0 - 1.0) / alpha;

  return temperature;
}

inline bool yaml_has_chipid(const std::string& yaml_file, std::string& chipid) {
  if (!file_exists(yaml_file)) return false;
  YAML::Node results = YAML::LoadFile(yaml_file);
  if (!results[chipid]) return false;
  YAML::Node entry = results[chipid];
  YAML::Node cal = entry["calib_constants"];

  return cal["CHIPID"].as<std::string>() == chipid;
}

void get_lpgbt_temps(Target* tgt);
