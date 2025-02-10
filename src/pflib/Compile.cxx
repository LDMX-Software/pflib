#include "pflib/Compile.h"
#include "pflib/Exception.h"

#include <map>
#include <iostream>
#include <algorithm>
#include <strings.h>

#include <yaml-cpp/yaml.h>
#include <algorithm>

namespace pflib {

/**
 * Structure holding a location in the registers
 */
struct RegisterLocation {
  /// the register the parameter is in (0-15)
  const int reg;
  /// the min bit the location is in the register (0-7)
  const int min_bit;
  /// the number of bits the location has in the register (1-8)
  const int n_bits;
  /// the mask for this number of bits
  const int mask;
  /**
   * Usable constructor
   *
   * This constructor allows us to calculat the mask from the
   * number of bits so that the downstream compilation functions
   * don't have to calculate it themselves.
   */
  RegisterLocation(int r, int m, int n)
    : reg{r}, min_bit{m}, n_bits{n},
      mask{((1 << n_bits) - 1)} {}
};

/**
 * A parameter for the HGC ROC includes one or more register locations
 * and a default value defined in the manual.
 */
struct Parameter {
  /// the default parameter value
  const int def;
  /// the locations that the parameter is split over
  const std::vector<RegisterLocation> registers;
  /// pass locations and default value of parameter
  Parameter(std::initializer_list<RegisterLocation> r,int def)
    : def{def}, registers{r} {}
  /// short constructor for single-location parameters
  Parameter(int r, int m, int n, int def)
    : Parameter({RegisterLocation(r,m,n)},def) {}
};

#include "register_maps/sipm_rocv2.h"

int str_to_int(std::string str) {
  if (str == "0") return 0;
  int base = 10;
  if (str[0] == '0' and str.length() > 2) {
    // binary or hexadecimal
    if (str[1] == 'b' or str[1] == 'B') {
      base = 2;
      str = str.substr(2);
    } else if (str[1] == 'x' or str[1] == 'X') {
      base = 16;
      str = str.substr(2);
    } 
  } else if (str[0] == '0' and str.length() > 1) {
    // octal
    base = 8;
    str = str.substr(1);
  }

  return std::stoi(str,nullptr,base);
}

std::string upper_cp(const std::string& str) {
  std::string STR{str};
  for (auto& c : STR) c = toupper(c);
  return STR;
}

void compile(const std::string& page_name, const std::string& param_name, const int& val, 
    std::map<int,std::map<int,uint8_t>>& register_values) {
  const auto& page_id {PARAMETER_LUT.at(page_name).first};
  const Parameter& spec{PARAMETER_LUT.at(page_name).second.at(param_name)};
  std::size_t value_curr_min_bit{0};
  for (const RegisterLocation& location : spec.registers) {
    // grab sub value of parameter in this register
    uint8_t sub_val = ((val >> value_curr_min_bit) & location.mask);
    value_curr_min_bit += location.n_bits;
    if (register_values[page_id].find(location.reg) == register_values[page_id].end()) {
      // initialize register value to zero if it hasn't been touched before
      register_values[page_id][location.reg] = 0;
    } else {
      // make sure to clear old value
      register_values[page_id][location.reg] &= ((location.mask << location.min_bit) ^ 0b11111111);
    }
    // put value into register at the specified location
    register_values[page_id][location.reg] += (sub_val << location.min_bit);
  } // loop over register locations
  return;
}

std::map<int,std::map<int,uint8_t>> 
compile(const std::string& page_name, const std::string& param_name, const int& val) {
  std::string PAGE_NAME(upper_cp(page_name)), PARAM_NAME(upper_cp(param_name));
  if (PARAMETER_LUT.find(PAGE_NAME) == PARAMETER_LUT.end()) {
    PFEXCEPTION_RAISE("NotFound", "The page named '"+PAGE_NAME+"' is not found in the look up table.");
  }
  const auto& page_lut{PARAMETER_LUT.at(PAGE_NAME).second};
  if (page_lut.find(PARAM_NAME) == page_lut.end()) {
    PFEXCEPTION_RAISE("NotFound", "The parameter named '"+PARAM_NAME
        +"' is not found in the look up table for page "+PAGE_NAME);
  }
  std::map<int,std::map<int,uint8_t>> rv;
  compile(PAGE_NAME, PARAM_NAME, val, rv);
  return rv;
}

std::map<int,std::map<int,uint8_t>> 
compile(const std::map<std::string,std::map<std::string,int>>& settings) {
  std::map<int,std::map<int,uint8_t>> register_values;
  for (const auto& page : settings) {
    // page.first => page name
    // page.second => parameter to value map
    std::string page_name = upper_cp(page.first);
    if (PARAMETER_LUT.find(page_name) == PARAMETER_LUT.end()) {
      // this exception shouldn't really ever happen because we check if the input
      // page matches any of the pages in the LUT in detail::apply, but
      // we leave this check in here for future development
      PFEXCEPTION_RAISE("NotFound", "The page named '"+page.first+"' is not found in the look up table.");
    }
    const auto& page_lut{PARAMETER_LUT.at(page_name).second};
    for (const auto& param : page.second) {
      // param.first => parameter name
      // param.second => value
      std::string param_name = upper_cp(param.first);
      if (page_lut.find(param_name) == page_lut.end()) {
        PFEXCEPTION_RAISE("NotFound", "The parameter named '"+param.first 
            +"' is not found in the look up table for page "+page.first);
      }
      compile(page_name, param_name, param.second, register_values);
    }   // loop over parameters in page
  }     // loop over pages

  return register_values;
}

std::map<std::string,std::map<std::string,int>>
decompile(const std::map<int,std::map<int,uint8_t>>& compiled_config, bool be_careful) {
  std::map<std::string,std::map<std::string,int>> settings;
  for (const auto& page : PARAMETER_LUT) {
    const std::string& page_name{page.first};
    const int& page_id{page.second.first};
    const auto& page_lut{page.second.second};
    if (compiled_config.find(page_id) == compiled_config.end()) {
      if (be_careful) {
        std::cerr << "WARNING: Page named " << page_name
          << " wasn't provided the necessary page " << page_id 
          << " to be deduced." << std::endl;
      }
      continue;
    }
    const auto& page_conf{compiled_config.at(page_id)};
    for (const auto& param : page_lut) {
      const Parameter& spec{page_lut.at(param.first)};
      std::size_t value_curr_min_bit{0};
      int pval{0};
      int n_missing_regs{0};
      for (const RegisterLocation& location : spec.registers) {
        uint8_t sub_val = 0; // defaults ot zero if not careful and register not found
        if (page_conf.find(location.reg) == page_conf.end()) {
          n_missing_regs++;
          if (be_careful) break;
        } else {
          // grab sub value of parameter in this register
          sub_val = ((page_conf.at(location.reg) >> location.min_bit) & location.mask);
        }
        pval += (sub_val << value_curr_min_bit);
        value_curr_min_bit += location.n_bits;
      }

      if (n_missing_regs == spec.registers.size() or (be_careful and n_missing_regs > 0)) {
        // skip this parameter
        if (be_careful) {
          std::cerr << "WARNING: Parameter " << param.first << " in page " << page_name
            << " wasn't provided the necessary registers to be deduced." << std::endl;
        }
      } else {
        settings[page_name][param.first] = pval;
      }
    }
  }

  return settings;
}

std::vector<std::string> parameters(const std::string& page) {
  static auto get_parameter_names = [&](const std::map<std::string,Parameter>& lut) -> std::vector<std::string> {
    std::vector<std::string> names;
    for (const auto& param : lut) names.push_back(param.first);
    return names;
  };

  std::string PAGE{upper_cp(page)};
  if (PAGE == "DIGITALHALF") {
    return get_parameter_names(DIGITAL_HALF_LUT);
  } else if (PAGE == "CHANNELWISE") {
    return get_parameter_names(CHANNEL_WISE_LUT);
  } else if (PAGE == "TOP" ) {
    return get_parameter_names(TOP_LUT);
  } else if (PAGE == "MASTERTDC") {
    return get_parameter_names(MASTER_TDC_LUT);
  } else if (PAGE == "REFERENCEVOLTAGE") {
    return get_parameter_names(REFERENCE_VOLTAGE_LUT);
  } else if (PAGE == "GLOBALANALOG") {
    return get_parameter_names(GLOBAL_ANALOG_LUT);
  } else if (PARAMETER_LUT.find(PAGE) != PARAMETER_LUT.end()) {
    return get_parameter_names(PARAMETER_LUT.at(PAGE).second);
  } else {
    PFEXCEPTION_RAISE("BadName", 
        "Input page name "+page+" does not match a page or type of page.");
  }
}

std::map<std::string,std::map<std::string,int>> defaults() {
  std::map<std::string,std::map<std::string,int>> settings;
  for(auto& page : PARAMETER_LUT) {
    for (auto& param : page.second.second) {
      settings[page.first][param.first] = param.second.def;
    }
  }
  return settings;
}

/**
 * Hidden functions to avoid users using them
 */
namespace detail {

/**
 * Extract a map of page_name, param_name to their values by crawling the YAML tree.
 *
 * @throw pflib::Exception if YAML file has a bad format (root node is not a map,
 * page nodes are not maps, page name doesn't match any pages, or parameter's value
 * does not exist).
 *
 * @param[in] params a YAML::Node to start extraction from
 * @param[in,out] settings map of names to values for extract parameters
 */
void extract(YAML::Node params, std::map<std::string,std::map<std::string,int>>& settings) {
  // deduce list of page names for search
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
      PFEXCEPTION_RAISE("BadFormat", "The YAML node for a page "+page_name+" is not a map.");
    }

    // apply these settings only to pages the input name
    std::vector<std::string> matching_pages;
    // determine matching function depending on format of input page
    //  if input page contains glob character '*', then match prefix,
    //  otherwise match entire word
    if (page_name.find('*') == std::string::npos) {
      std::string PAGE_NAME = upper_cp(page_name);
      std::copy_if(page_names.begin(), page_names.end(), std::back_inserter(matching_pages),
          [&](const std::string& page) { return PAGE_NAME == page; });
    } else {
      page_name = page_name.substr(0,page_name.find('*'));
      std::copy_if(page_names.begin(), page_names.end(), std::back_inserter(matching_pages),
          [&](const std::string& page) { 
          return strncasecmp(page.c_str(), page_name.c_str(), page_name.size()) == 0;
          });
    }


    if (matching_pages.empty()) {
      PFEXCEPTION_RAISE("NotFound", 
          "The page "+page_name+" does not match any pages in the look up table.");
    }

    for (const auto& page : matching_pages) {
      for (const auto& param : page_settings) {
        std::string sval = param.second.as<std::string>();
        if (sval.empty()) {
          PFEXCEPTION_RAISE("BadFormat",
              "Non-existent value for parameter "+param.first.as<std::string>());
        }
        std::string param_name = upper_cp(param.first.as<std::string>());
        settings[page][param_name] = str_to_int(sval);
      }
    }
  }
}

} // namespace detail

void extract(const std::vector<std::string>& setting_files,
    std::map<std::string,std::map<std::string,int>>& settings) {
  for (auto& setting_file : setting_files) {
    YAML::Node setting_yaml;
    try {
       setting_yaml = YAML::LoadFile(setting_file);
    } catch (const YAML::BadFile& e) {
      PFEXCEPTION_RAISE("BadFile","Unable to load file " + setting_file);
    }
    if (setting_yaml.IsSequence()) {
      for (std::size_t i{0}; i < setting_yaml.size(); i++) detail::extract(setting_yaml[i],settings);
    } else {
      detail::extract(setting_yaml, settings);
    }
  }
}

std::map<int,std::map<int,uint8_t>> 
compile(const std::vector<std::string>& setting_files, bool prepend_defaults) {
  std::map<std::string,std::map<std::string,int>> settings;
  // if we prepend the defaults, put all settings and their defaults 
  // into the settings map before extraction
  if (prepend_defaults) { settings = defaults(); }
  extract(setting_files,settings);
  return compile(settings);
}

std::map<int,std::map<int,uint8_t>> 
compile(const std::string& setting_file, bool prepend_defaults) {
  return compile(std::vector<std::string>{setting_file}, prepend_defaults);
}
}
