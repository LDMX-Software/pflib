#include "inv_vref_scan_lund.h"

#include <yaml-cpp/yaml.h>

#include <fstream>

#include "../algorithm/inv_vref_lund.h"

void inv_vref_scan_lund(Target* tgt) {
  auto settings = pflib::algorithm::inv_vref_lund(tgt);
  for (const auto& [i_roc, parameters] : settings) {
	auto roc{tgt->roc(i_roc)};
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

  	if (pftool::readline_bool("View deduced settings? ", true)) {
    	std::cout << out.c_str() << std::endl;
  	}

  	if (pftool::readline_bool("Apply settings to the chip? ", true)) {
    	roc.applyParameters(parameters);
  	}

  	if (pftool::readline_bool("Save settings to a file? ", false)) {
    	std::string fname = pftool::readline_path(
        	"inv_vref_scan_lund-" + std::to_string(i_roc) +
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
