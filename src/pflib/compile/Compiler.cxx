#include "pflib/compile/Compiler.h"
#include "pflib/Exception.h"

#include <map>
#include <regex>

#include <yaml-cpp/yaml.h>

namespace pflib::compile {

/**
 * Structure holding a location in the registers
 */
struct RegisterLocation {
  /// the register the parameter is in (0-15)
  const short reg;
  /// the min bit the location is in the register (0-7)
  const short min_bit;
  /// the number of bits the location has in the register (1-8)
  const short n_bits;
  /// the mask for this number of bits
  const short mask;
  /**
   * Usable constructor
   */
  RegisterLocation(short r, short m, short n)
    : reg{r}, min_bit{m}, n_bits{n},
      mask{((static_cast<short>(1) << n_bits) - static_cast<short>(1))} {}
};

struct Parameter {
  /// the default parameter value
  const int def;
  /// the locations that the parameter is split over
  const std::vector<RegisterLocation> registers;

  Parameter(int def, std::initializer_list<RegisterLocation> r)
    : def{def}, registers{r} {}
};

const std::map<std::string,std::pair<int,std::map<std::string,Parameter>>>
PARAMETER_LUT = {
  {"Global_Analog_0", { 297, {
                        {"ON_dac_trim", Parameter(1,{RegisterLocation(0,0,1)})}
                      }}
  }
};

void Compiler::apply(YAML::Node params) {
  // deduce list of page names for regex search
  //    only do this once per program run
  static std::vector<std::string> page_names;
  if (page_names.empty()) {
    for (auto& page : PARAMETER_LUT) page_names.push_back(page.first);
  }

  if (not params.IsMap()) {
    PFEXCEPTION_RAISE("BadFormat", "The YAML node provided is not a map.");
  }

  for (const auto& page_pair : params) {
    std::string page_name = page_pair.first.as<std::string>();
    YAML::Node page_settings = page_pair.second;
    if (not page_settings.IsMap()) {
      PFEXCEPTION_RAISE("BadFormat", "The YAML node for a specific page is not a map.");
    }

    // apply these settings only to pages that match the regex
    std::regex page_regex(page_name);
    for (const auto& page : page_names) {
      if (not std::regex_match(page,page_regex)) continue;

      for (const auto& param : page_settings)
        settings_[page][param.first.as<std::string>()] = param.second.as<int>();
    }
  }
}

std::map<int,std::map<int,uint8_t>> Compiler::compile() {
  std::map<int,std::map<int,uint8_t>> register_values;
  for (const auto& page : settings_) {
    // page.first => page name
    // page.second => parameter to value map
    if (PARAMETER_LUT.find(page.first) == PARAMETER_LUT.end()) {
      PFEXCEPTION_RAISE("NotFound", "The page named '"+page.first+"' is not found in the look up table.");
    }
    const auto& page_id{PARAMETER_LUT.at(page.first).first};
    const auto& page_lut{PARAMETER_LUT.at(page.first).second};
    for (const auto& param : page.second) {
      // param.first => parameter name
      // param.second => value
      if (page_lut.find(param.first) == page_lut.end()) {
        PFEXCEPTION_RAISE("NotFound", "The parameter named '"+param.first 
            +"' is not found in the look up table for page "+page.first);
      }

      const Parameter& spec{page_lut.at(param.first)};
      std::size_t value_curr_min_bit{0};
      for (const RegisterLocation& location : spec.registers) {
        // grab sub value of parameter in this register
        uint8_t sub_val = ((param.second >> value_curr_min_bit) & location.mask);
        value_curr_min_bit += location.n_bits;
        // initialize register value to zero if it hasn't been touched before
        if (register_values[page_id].find(location.reg) == register_values[page_id].end()) {
          register_values[page_id][location.reg] = 0;
        }

        // put value into register at the specified location
        register_values[page_id][location.reg] += (sub_val << location.min_bit);
      } // loop over register locations
    }   // loop over parameters in page
  }     // loop over pages

  return register_values;
}

Compiler::Compiler(const std::vector<std::string>& setting_files, bool prepend_defaults) {
  // if we prepend the defaults, put all settings and their defaults 
  // into the settings map
  if (prepend_defaults) {
    for(auto& page : PARAMETER_LUT) {
      for (auto& param : page.second.second) {
        settings_[page.first][param.first] = param.second.def;
      }
    }
  }

  for (auto& setting_file : setting_files) {
    YAML::Node setting_yaml = YAML::LoadFile(setting_file);
    if (setting_yaml.IsSequence()) {
      for (std::size_t i{0}; i < setting_yaml.size(); i++) apply(setting_yaml[i]);
    } else {
      apply(setting_yaml);
    }
  }
}

}
