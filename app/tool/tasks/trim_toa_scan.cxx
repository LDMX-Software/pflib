#include "trim_toa_scan.h"

#include <yaml-cpp/yaml.h>

#include <fstream>

#include "pflib/algorithm/trim_toa_scan.h"

void trim_toa_scan(Target* tgt) {
  auto roc{tgt->hcal().roc(pftool::state.iroc)};
  auto settings = pflib::algorithm::trim_toa_scan(tgt, roc);
  YAML::Emitter out;
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

  if (pftool::readline_bool("View deduced settings? ", true)) {
    std::cout << out.c_str() << std::endl;
  }

  if (pftool::readline_bool("Apply settings to the chip? ", true)) {
    roc.applyParameters(settings);
  }

  if (pftool::readline_bool("Save settings to a file? ", false)) {
    std::string fname = pftool::readline_path(
        "trim-toa-scan-" + std::to_string(pftool::state.iroc) + "-settings",
        ".yaml");

    std::ofstream f{fname};
    if (not f.is_open()) {
      PFEXCEPTION_RAISE("File", "Unable to open file " + fname + ".");
    }
    f << out.c_str() << std::endl;
  }
}
