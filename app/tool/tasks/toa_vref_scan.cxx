/**
 * @file
 * \addtogroup pftool
 * @{
 * \addtogroup tasks
 * @{
 */
#include "toa_vref_scan.h"

#include <yaml-cpp/yaml.h>

#include <fstream>

#include "../algorithm/toa_vref_scan.h"

void toa_vref_scan(Target* tgt) {
  bool scan_all = pftool::readline_bool("Scan all VREF values?", false);

  bool write_csv =
      pftool::readline_bool("Save raw scan data to a CSV file?", true);
  std::string csv_path = "";
  if (write_csv) {
    csv_path = pftool::readline_path("toa-vref-scan-data", ".csv");
  }

  auto settings =
      pflib::algorithm::toa_vref_scan(tgt, scan_all, write_csv, csv_path);

  YAML::Emitter out;
  out << YAML::BeginMap;

  for (const auto& [i_roc, page_map] : settings) {
    out << YAML::Key << i_roc;
    out << YAML::Value << YAML::BeginMap;
    for (const auto& page : page_map) {
      out << YAML::Key << page.first;
      out << YAML::Value << YAML::BeginMap;
      for (const auto& param : page.second) {
        out << YAML::Key << param.first << YAML::Value << param.second;
      }
      out << YAML::EndMap;
    }
    out << YAML::EndMap;
  }
  out << YAML::EndMap;

  if (pftool::readline_bool("View deduced settings? ", true)) {
    std::cout << out.c_str() << std::endl;
  }

  if (pftool::readline_bool("Apply settings to the chips? ", true)) {
    for (const auto& [i_roc, page_map] : settings) {
      tgt->roc(i_roc).applyParameters(page_map);
    }
  }

  if (pftool::readline_bool("Save settings to a file? ", false)) {
    std::string fname =
        pftool::readline_path("toa-vref-scan-settings", ".yaml");

    std::ofstream f{fname};
    if (not f.is_open()) {
      PFEXCEPTION_RAISE("File", "Unable to open file " + fname + ".");
    }
    f << out.c_str() << std::endl;
  }
}
/**
 * @}
 * @}
 */