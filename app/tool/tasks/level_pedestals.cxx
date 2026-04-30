#include "level_pedestals.h"

#include <yaml-cpp/yaml.h>

#include <fstream>

#include "../algorithm/level_pedestals.h"

void level_pedestals(Target* tgt) {
  auto settings = pflib::algorithm::level_pedestals(tgt);
  for (const auto& [iroc, parameters]: settings) {
    YAML::Emitter out;
    out << YAML::BeginMap;
    for (const auto& page : parameters) {
      out << YAML::Key << page.first;
      out << YAML::Value << YAML::BeginMap;
      for (const auto& param : page.second) {
        out << YAML::Key << param.first << YAML::Value << param.second;
      }
      out << YAML::EndMap;
    }
    out << YAML::EndMap;

    std::string iroc_str{std::to_string(iroc)};

    if (pftool::readline_bool("View deduced settings for ROC"+iroc_str+"? ", true)) {
      std::cout << out.c_str() << std::endl;
    }

    if (pftool::readline_bool("Apply settings to ROC"+iroc_str+"? ", false)) {
      tgt->roc(iroc).applyParameters(parameters);
    }

    if (pftool::readline_bool("Save settings to a file? ", false)) {
      std::string fname = pftool::readline_path(
          "level-pedestals-roc-" + std::to_string(iroc) +
              "-settings",
          ".yaml");

      std::ofstream f{fname};
      if (not f.is_open()) {
        PFEXCEPTION_RAISE("File", "Unable to open file " + fname + ".");
      }
      f << out.c_str() << std::endl;
    }
  }
}
