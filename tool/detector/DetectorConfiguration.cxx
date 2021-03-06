#include "DetectorConfiguration.h"

#include "pflib/Compile.h"
#include "pflib/PolarfireTarget.h"

#ifdef PFTOOL_ROGUE
#include "pflib/rogue/RogueWishboneInterface.h"
#endif

#ifdef PFTOOL_UHAL
#include "pflib/uhal/uhalWishboneInterface.h"
#endif

#include <yaml-cpp/yaml.h>
#include <string.h>
#include <iostream>

/******************************************************************************
 * Definition of settings
 *****************************************************************************/
/// set the calib offset to the value
class calib_offset : public pflib::detail::PolarfireSetting {
  int val_;
 public: 
  virtual void import(YAML::Node val) final override {
    val_ = val.as<int>();
  }
  virtual void execute(pflib::PolarfireTarget* pft) final override {
    // set calib offset
    int len, offset;
    pft->backend->fc_get_setup_calib(len,offset);
    offset = val_;
    pft->backend->fc_setup_calib(len,offset);
  }
  virtual void stream(std::ostream& s) final override {
    s << val_;
  }
};

/// sets the sipm bias adc value for all connectors on the input rocs
class sipm_bias : public pflib::detail::PolarfireSetting {
  int val_;
  std::vector<int> rocs_;
 public:
  virtual void import(YAML::Node val) final override {
    if (not val.IsMap()) {
      PFEXCEPTION_RAISE("BadFormat","SipmBias expects a map value.");
    }
    /// notice we do nothing if an argument is not given,
    //    this is so the value can be "inherited" from parent
    //    'inherit' yaml nodes and then only modified below
    if (val["value"]) val_ = val["value"].as<int>();
    if (val["rocs"]) rocs_ = val["rocs"].as<std::vector<int>>();
  }
  virtual void execute(pflib::PolarfireTarget* pft) final override {
    for (auto& roc : rocs_) {
      for (int connector{0}; connector < 16; connector++) {
        pft->setBiasSetting(roc, 0, connector, val_);
      }
    }
  }
  virtual void stream(std::ostream& s) final override {
    s << val_ << " on rocs ";
    for (auto& r : rocs_) s << r << " ";
  }
};

/******************************************************************************
 * Registration of available settings
 *    the string passed as an argument to declare is the name of the setting
 *    as read from the YAML file
 *
 *    In addition to the names listed here, 'inherit' and 'hgcrocs' are also
 *    taken as reserved names for other purposes below.
 *****************************************************************************/
namespace {
  auto& factory{pflib::detail::PolarfireSetting::Factory::get()};
  auto v0 = factory.declare<calib_offset>("calib_offset");
  auto v1 = factory.declare<sipm_bias>("sipm_bias");
}

/******************************************************************************
 * Implementation Layer Below, change carefully!
 *****************************************************************************/
namespace pflib {

static const std::string INHERIT = "INHERIT";
static const std::string HGCROCS = "HGCROCS";

void DetectorConfiguration::PolarfireConfiguration::import(YAML::Node conf) {
  if (not conf.IsMap()) {
    PFEXCEPTION_RAISE("BadFormat","YAML node for polarfire not a map.");
  }

  for (const auto& setting_pair : conf) {
    std::string setting = setting_pair.first.as<std::string>();
    if (strcasecmp(HGCROCS.c_str(), setting.c_str()) == 0) {
      for (const auto& sub_pair : setting_pair.second) {
        std::string roc = sub_pair.first.as<std::string>();
        // skip undefined target node (just being listed for inherit)
        if (not sub_pair.second or sub_pair.second.IsNull()) continue;
        YAML::Node roc_params{sub_pair.second};
        if (not sub_pair.second.IsMap()) {
          auto roc_filename = sub_pair.second.as<std::string>();
          roc_params = YAML::LoadFile(roc_filename);
        }
        if (strcasecmp(INHERIT.c_str(), roc.c_str()) != 0) {
          detail::extract(sub_pair.second, hgcrocs_[sub_pair.first.as<int>()]);
        } else {
          for (auto& roc_pair : hgcrocs_) {
            detail::extract(sub_pair.second, roc_pair.second);
          }
        }
      }
    } else {
      // throws exception if setting not found
      settings_[setting] = detail::PolarfireSetting::Factory::get().make(setting);
      settings_[setting]->import(setting_pair.second);
    }
  }
}

void DetectorConfiguration::PolarfireConfiguration::apply(const std::string& host) {
#ifdef PFTOOL_ROGUE
  auto pft = std::make_unique<PolarfireTarget>(new pflib::rogue::RogueWishboneInterface(host,5970));

  for (auto& setting : settings_) {
    setting.second->execute(pft.get());
  }

  for (auto& hgcroc : hgcrocs_) {
    // general HGC ROC parameters
    pft->hcal.roc(hgcroc.first).applyParameters(hgcroc.second);
  }
#else
  // no good reason just rushing
  PFEXCEPTION_RAISE("NoImpl",
      "DetectorConfiguratin::apply has not been implemented for uHAL systems.");

#endif
}

DetectorConfiguration::DetectorConfiguration(const std::string& config) try {
  YAML::Node config_yaml;
  try {
    config_yaml = YAML::LoadFile(config);
  } catch (const YAML::BadFile& e) {
    PFEXCEPTION_RAISE("BadFile", "Unable to load file "+config);
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
      polarfires_[hostname];
      if (not polarfire_pair.second.IsMap()) {
        PFEXCEPTION_RAISE("BadFormat","The YAML node for polarfire "+hostname+" is not a map.");
      }
      for (const auto& setting_pair : polarfire_pair.second) {
        if (strcasecmp(HGCROCS.c_str(),setting_pair.first.as<std::string>().c_str()) == 0) {
          for (const auto& sub_pair : setting_pair.second) {
            std::string roc = sub_pair.first.as<std::string>();
            if (strcasecmp(INHERIT.c_str(), roc.c_str()) != 0) {
              polarfires_[hostname].hgcrocs_[sub_pair.first.as<int>()];
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
      for (auto& pf_pair : polarfires_) {
        pf_pair.second.import(polarfire_settings);
      }
    } else {
      // specific polarfire
      polarfires_[hostname].import(polarfire_settings);
    }
  }
} catch (const YAML::Exception& e) {
  PFEXCEPTION_RAISE("BadFormat", e.what());
}

void DetectorConfiguration::apply() {
  for (auto& polarfire : polarfires_) {
    polarfire.second.apply(polarfire.first);
  }
}

void DetectorConfiguration::stream(std::ostream& s) const {
  for (const auto& polarfire_pair : polarfires_) {
    s << polarfire_pair.first << "\n";
    for (const auto& roc_pair : polarfire_pair.second.hgcrocs_) {
      s << "  ROC " << roc_pair.first << "\n";
      for (const auto& page_pair : roc_pair.second) {
        s << "    " << page_pair.first << "\n";
        for (const auto& param_pair : page_pair.second) {
          s << "      " << param_pair.first << " : " << param_pair.second << "\n";
        }
      }
    }
    for (const auto& setting : polarfire_pair.second.settings_) {
      s << "  " << setting.first << " : ";
      setting.second->stream(s);
      s << "\n";
    }
  }
  s << std::flush;
}

}

std::ostream& operator<<(std::ostream& s, const pflib::DetectorConfiguration& dc) {
  dc.stream(s);
  return s;
}
