#include "pflib/DetectorConfiguration.h"
#include "pflib/Compile.h"
#include "pflib/PolarfireTarget.h"

#include <yaml-cpp/yaml.h>
#include <string.h>

namespace pflib {

static const std::string INHERIT = "INHERIT";
static const std::string HGCROCS = "HGCROCS";
static const std::string CALIB_OFFSET = "CALIB_OFFSET";
static const std::string SIPM_BIAS = "SIPM_BIAS";

void DetectorConfiguration::PolarfireConfiguration::import(YAML::Node conf) {
  if (not conf.IsMap()) {
    PFEXCEPTION_RAISE("BadFormat","YAML node for polarfire not a map.");
  }

  for (const auto& setting_pair : conf) {
    std::string setting = setting_pair.first.as<std::string>();
    if (strcasecmp(CALIB_OFFSET.c_str(), setting.c_str()) == 0) {
      calib_offset = setting_pair.second.as<int>();
    } else if (strcasecmp(SIPM_BIAS.c_str(), setting.c_str()) == 0) {
      sipm_bias = setting_pair.second.as<int>();
    } else if (strcasecmp(HGCROCS.c_str(), setting.c_str()) == 0) {
      for (const auto& sub_pair : setting_pair.second) {
        std::string roc = sub_pair.first.as<std::string>();
        std::cout << " " << roc << std::endl;
        if (not sub_pair.second.IsMap()) continue;
        if (strcasecmp(INHERIT.c_str(), roc.c_str()) != 0) {
          detail::extract(sub_pair.second, hgcrocs[sub_pair.first.as<int>()]);
        } else {
          for (auto& roc_pair : hgcrocs) {
            detail::extract(sub_pair.second, roc_pair.second);
          }
        }
      }
    } else {
      PFEXCEPTION_RAISE("BadFormat", "Unrecognized polarfire setting "+setting);
    }
  }
}

DetectorConfiguration::DetectorConfiguration(const std::string& config) {
  YAML::Node config_yaml;
  try {
    config_yaml = YAML::LoadFile(config);
  } catch (const YAML::BadFile& e) {
    PFEXCEPTION_RAISE("BadFile", "Unable to load file "+config);
  } catch (const YAML::ParserException& e) {
    PFEXCEPTION_RAISE("BadFormat", e.what());
  }
  
  if (not config_yaml.IsMap()) {
    PFEXCEPTION_RAISE("BadFormat", "The provided YAML file is not a map.");
  }
  
  /****************************************************************************
   * Initialization, get the polarfires and the rocs on those polarfires
   * that we will be configuring by crawling the YAML tree once
   ***************************************************************************/
  for (const auto& polarfire_pair : config_yaml) {
    std::string hostname = polarfire_pair.first.as<std::string>();
    if (strcasecmp(INHERIT.c_str(), hostname.c_str()) != 0) {
      polarfires[hostname];
      if (not polarfire_pair.second.IsMap()) {
        PFEXCEPTION_RAISE("BadFormat","The YAML node for polarfire "+hostname+" is not a map.");
      }
      for (const auto& setting_pair : polarfire_pair.second) {
        if (strcasecmp(HGCROCS.c_str(),setting_pair.first.as<std::string>().c_str()) == 0) {
          for (const auto& sub_pair : setting_pair.second) {
            std::string roc = sub_pair.first.as<std::string>();
            if (strcasecmp(INHERIT.c_str(), roc.c_str()) != 0) {
              polarfires[hostname].hgcrocs[sub_pair.first.as<int>()];
            }
          }
        }
      }
    }
  }

  /****************************************************************************
   * Setting Extraction
   * Now we can extract the parameters, appropriately applying global 'inherit'
   * settings to allow downstream objects
   ***************************************************************************/
  for (const auto& polarfire_pair : config_yaml) {
    std::string hostname = polarfire_pair.first.as<std::string>();
    YAML::Node polarfire_settings = polarfire_pair.second;
    if (not polarfire_settings.IsMap()) {
      PFEXCEPTION_RAISE("BadFormat","The YAML node for polarfire "+hostname+" is not a map.");
    }

    if (strcasecmp(INHERIT.c_str(), hostname.c_str()) == 0) {
      // apply to all polarfires
      for (auto& pf_pair : polarfires) {
        pf_pair.second.import(polarfire_settings);
      }
    } else {
      // specific polarfire
      polarfires[hostname].import(polarfire_settings);
    }
  }
}

void DetectorConfiguration::apply() {
}

void DetectorConfiguration::stream(std::ostream& s) const {
  for (const auto& polarfire_pair : polarfires) {
    s << polarfire_pair.first << "\n";
    s << "  " << CALIB_OFFSET << ": " << polarfire_pair.second.calib_offset << "\n";
    s << "  " << SIPM_BIAS << ": " << polarfire_pair.second.sipm_bias << "\n";
    for (const auto& roc_pair : polarfire_pair.second.hgcrocs) {
      s << "  ROC " << roc_pair.first << "\n";
      for (const auto& page_pair : roc_pair.second) {
        s << "    " << page_pair.first << "\n";
        for (const auto& param_pair : page_pair.second) {
          s << "      " << param_pair.first << " : " << param_pair.second << "\n";
        }
      }
    }
  }
  s << std::flush;
}

}

std::ostream& operator<<(std::ostream& s, const pflib::DetectorConfiguration& dc) {
  dc.stream(s);
  return s;
}
