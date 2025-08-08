#include "toa_vref_scan.h"

#include <fstream>
#include <yaml-cpp/yaml.h>
#include "pflib/algorithm/find_toa_vref.h"

void toa_vref_scan(Target* tgt) {
  auto roc{tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version())};
  auto settings = pflib::algorithm::find_toa_vref(tgt, roc);
  YAML::Emitter out;
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

  if (pftool::readline_bool("Apple settings to the chip? ", true)) {
    roc.applyParameters(settings);
  }

  if (pftool::readline_bool("Save settings to a file? ", false)) {
    std::string fname = pftool::readline_path(
      "toa-vref-scan-"+std::to_string(pftool::state.iroc)+"-settings",
      ".yaml"
    );

    std::ofstream f{fname};
    if (not f.is_open()) {
      PFEXCEPTION_RAISE(
        "File",
        "Unable to open file " + fname + "."
      );
    }
    f << out.c_str() << std::endl;
  }
}
