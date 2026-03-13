#include "set_delays.h"
#include "../algorithm/delay_scan.h"
#include "../daq_run.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

void set_delays(Target* tgt) {

  auto roc{tgt->roc(pftool::state.iroc)};

  std::map<std::string, std::map<std::string, uint64_t>> settings;

  if (pftool::state.daq_format_mode == Target::DaqFormat::SIMPLEROC) {
    settings = pflib::algorithm::delay_scan<pflib::packing::SingleROCEventPacket>(tgt, roc);
  } else if (pftool::state.daq_format_mode == Target::DaqFormat::ECOND_SW_HEADERS) {
    settings = pflib::algorithm::delay_scan<pflib::packing::MultiSampleECONDEventPacket>(tgt, roc);
  }
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
        "set-delays" + std::to_string(pftool::state.iroc) + "-settings",
        ".yaml");

    std::ofstream f{fname};
    if (not f.is_open()) {
      PFEXCEPTION_RAISE("File", "Unable to open file " + fname + ".");
    }
    f << out.c_str() << std::endl;
  }
}
