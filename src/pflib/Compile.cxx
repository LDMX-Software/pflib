#include "pflib/Compile.h"

#include <strings.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <numeric>

#include <cinttypes>

#include "pflib/Exception.h"

namespace pflib {

std::string upper_cp(const std::string& str) {
  std::string STR{str};
  for (auto& c : STR) c = toupper(c);
  return STR;
}

#include "register_maps/register_maps.h"

Compiler Compiler::get(const std::string& type_version) {
   // Try ROC type map first
  auto roc_it = REGISTER_MAP_BY_ROC_TYPE.find(type_version);
  if (roc_it != REGISTER_MAP_BY_ROC_TYPE.end()) {
    return Compiler(roc_it->second.second, roc_it->second.first);
  }

  // Try ECON type map next
  auto econ_it = REGISTER_MAP_BY_ECON_TYPE.find(type_version);
  if (econ_it != REGISTER_MAP_BY_ECON_TYPE.end()) {
    return Compiler(econ_it->second.second, econ_it->second.first);

  }
  
  PFEXCEPTION_RAISE("BadType", "Type_version " + type_version +
                    " is not present within ROC or ECON register maps.");
}

Compiler::Compiler(const ParameterLUT& parameter_lut, const PageLUT& page_lut)
    : parameter_lut_{parameter_lut}, page_lut_{page_lut} {}

/**
 * Calculate the Most Significant Bit of the input unsigned integer
 *
 * This is not safe and could be very slow if the value input is malformed
 * (i.e. not a safely constructed unsigned integer).
 */
std::size_t msb(uint32_t v) {
  int r{0};
  while (v >>= 1) {
    r++;
  }
  return r;
}

void Compiler::compile(const std::string& page_name,
                       const std::string& param_name, const uint64_t val,
                       std::map<int, std::map<int, uint8_t>>& register_values) {
  std::cout << "[uint64_t overload] value = " << val
	    << " sizeof=" << sizeof(val) << std::endl;
   
  std::cout << "Setting parameter " << page_name << "." << param_name
            << " = " << val
            << " (MSB=" << (64 - __builtin_clzll(val)) << ", total bits=64)\n";
  std::cout << "sizeof(val) = " << sizeof(val) << std::endl;

  for (const auto& p : parameter_lut_) {
    std::cout << "Page: " << p.first << "\n";
    for (const auto& param : p.second.second) {
      std::cout << "  Param: " << param.first << "\n";
    }
  }
  
  const auto& page_id{parameter_lut_.at(page_name).first};
  const Parameter& spec{parameter_lut_.at(page_name).second.at(param_name)};
  uint64_t uval{static_cast<uint64_t>(val)};
  //uint32_t uval{static_cast<uint32_t>(val)};

  std::size_t total_nbits =
      std::accumulate(spec.registers.begin(), spec.registers.end(), 0,
                      [](std::size_t bit_count, const RegisterLocation& rhs) {
                        return bit_count + rhs.n_bits;
                      });
  std::size_t val_msb = msb(uval);
  printf("Setting parameter %s.%s = %" PRIu64 " (MSB=%zu, total bits=%zu)\n",
       page_name.c_str(), param_name.c_str(), uval, val_msb, total_nbits);

  if (val_msb >= total_nbits) {
    std::stringstream msg;
    msg << "Parameter " << page_name << '.' << param_name
        << " is being set to a value (" << val << ") exceeding its size ("
        << total_nbits << " bits -> value < " << (1u << total_nbits) << ")";
    PFEXCEPTION_RAISE("ValOver", msg.str());
  }

  std::size_t value_curr_min_bit{0};
  printf("%s.%s -> page %u\n", page_name.c_str(), param_name.c_str(), page_id);
  pflib_log(trace) << page_name << "." << param_name << " -> page " << page_id;
  for (const RegisterLocation& location : spec.registers) {
    // grab sub value of parameter in this register
    uint8_t sub_val = ((uval >> value_curr_min_bit) & location.mask);
    uint64_t sub_val64 = ((uval >> value_curr_min_bit) & location.mask);
    printf("  Sub-value debug: %" PRIu64 " -> 0x%02" PRIX64 "\n", sub_val64, sub_val64);
    printf("  Sub-value: %u (uval %u) (mask=0x%02X, shift=%zu) -> register 0x%04X\n",
	   sub_val, uval, location.mask, location.min_bit, location.reg);

    pflib_log(trace) << "  " << sub_val << " at " << location.reg << ", "
                     << location.n_bits << " bits";
    value_curr_min_bit += location.n_bits;
    if (register_values[page_id].find(location.reg) ==
        register_values[page_id].end()) {
      // initialize register value to zero if it hasn't been touched before
      register_values[page_id][location.reg] = 0;
      printf("    Initialized register 0x%04X to 0\n", location.reg);
    } else {
      // make sure to clear old value
      register_values[page_id][location.reg] &=
          ((location.mask << location.min_bit) ^ 0b11111111);
      printf("    Cleared old bits in register 0x%04X\n", location.reg);
    }
    // put value into register at the specified location
    register_values[page_id][location.reg] += (sub_val << location.min_bit);

    printf("    Updated register 0x%04X = 0x%02X\n", location.reg,
	   register_values[page_id][location.reg]);
  }  // loop over register locations
  printf("Finished setting %s.%s\n", page_name.c_str(), param_name.c_str());
  return;
}

std::map<int, std::map<int, uint8_t>> Compiler::compile(
    const std::string& page_name, const std::string& param_name,
    const int& val) {
  std::string PAGE_NAME(upper_cp(page_name)), PARAM_NAME(upper_cp(param_name));
  if (parameter_lut_.find(PAGE_NAME) == parameter_lut_.end()) {
    PFEXCEPTION_RAISE("NotFound", "The page named '" + PAGE_NAME +
                                      "' is not found in the look up table.");
  }
  const auto& page_lut{parameter_lut_.at(PAGE_NAME).second};
  if (page_lut.find(PARAM_NAME) == page_lut.end()) {
    PFEXCEPTION_RAISE("NotFound",
                      "The parameter named '" + PARAM_NAME +
                          "' is not found in the look up table for page " +
                          PAGE_NAME);
  }
  std::map<int, std::map<int, uint8_t>> rv;
  compile(PAGE_NAME, PARAM_NAME, val, rv);
  return rv;
}

std::map<int, std::map<int, uint8_t>> Compiler::compile(
    const std::map<std::string, std::map<std::string, int>>& settings) {
  std::map<int, std::map<int, uint8_t>> register_values;
  for (const auto& page : settings) {
    // page.first => page name
    // page.second => parameter to value map
    std::string page_name = upper_cp(page.first);
    if (parameter_lut_.find(page_name) == parameter_lut_.end()) {
      // this exception shouldn't really ever happen because we check if the
      // input page matches any of the pages in the LUT in detail::apply, but we
      // leave this check in here for future development
      PFEXCEPTION_RAISE("NotFound", "The page named '" + page.first +
                                        "' is not found in the look up table.");
    }
    const auto& page_lut{parameter_lut_.at(page_name).second};
    for (const auto& param : page.second) {
      // param.first => parameter name
      // param.second => value
      std::string param_name = upper_cp(param.first);
      if (page_lut.find(param_name) == page_lut.end()) {
        PFEXCEPTION_RAISE("NotFound",
                          "The parameter named '" + param.first +
                              "' is not found in the look up table for page " +
                              page.first);
      }
      compile(page_name, param_name, param.second, register_values);
    }  // loop over parameters in page
  }  // loop over pages

  return register_values;
}

std::vector<int> Compiler::get_known_pages() {
  std::vector<int> known_pages;
  known_pages.reserve(parameter_lut_.size());
  for (const auto& [page_name, page_spec] : parameter_lut_) {
    known_pages.push_back(page_spec.first);
  }
  return known_pages;
}

std::map<std::string, std::map<std::string, int>> Compiler::decompile(
    const std::map<int, std::map<int, uint8_t>>& compiled_config,
    bool be_careful) {
  std::map<std::string, std::map<std::string, int>> settings;
  for (const auto& page : parameter_lut_) {
    const std::string& page_name{page.first};
    const int& page_id{page.second.first};
    const auto& page_lut{page.second.second};
    if (compiled_config.find(page_id) == compiled_config.end()) {
      if (be_careful) {
        pflib_log(warn) << "page " << page_name
                        << " wasn't provided the necessary page " << page_id
                        << " to be deduced";
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
        uint8_t sub_val =
            0;  // defaults ot zero if not careful and register not found
        if (page_conf.find(location.reg) == page_conf.end()) {
          n_missing_regs++;
          if (be_careful) break;
        } else {
          // grab sub value of parameter in this register
          sub_val = ((page_conf.at(location.reg) >> location.min_bit) &
                     location.mask);
        }
        pval += (sub_val << value_curr_min_bit);
        value_curr_min_bit += location.n_bits;
      }

      if (n_missing_regs == spec.registers.size() or
          (be_careful and n_missing_regs > 0)) {
        // skip this parameter
        if (be_careful) {
          pflib_log(warn)
              << "parameter " << param.first << " in page " << page_name
              << " wasn't provided the necessary registers to be deduced";
        }
      } else {
        settings[page_name][param.first] = pval;
      }
    }
  }

  return settings;
}

std::map<int, std::map<int, uint8_t>> Compiler::getRegisters(
    const std::string& page) {
  std::string PAGE{upper_cp(page)};
  auto page_it{parameter_lut_.find(PAGE)};
  if (page_it == parameter_lut_.end()) {
    PFEXCEPTION_RAISE("BadPage", "Input page " + page +
                                     " is not present in the look up table.");
  }
  std::map<int, std::map<int, uint8_t>> registers;
  for (const auto& param : page_it->second.second) {
    compile(page, param.first, 0, registers);
  }
  return registers;
}

std::map<std::string, std::map<std::string, int>> Compiler::defaults() {
  std::map<std::string, std::map<std::string, int>> settings;
  for (auto& page : parameter_lut_) {
    for (auto& param : page.second.second) {
      settings[page.first][param.first] = param.second.def;
    }
  }
  return settings;
}

void Compiler::extract(
    const std::vector<std::string>& setting_files,
    std::map<std::string, std::map<std::string, int>>& settings) {
  for (auto& setting_file : setting_files) {
    YAML::Node setting_yaml;
    try {
      setting_yaml = YAML::LoadFile(setting_file);
    } catch (const YAML::BadFile& e) {
      PFEXCEPTION_RAISE("BadFile", "Unable to load file " + setting_file);
    }
    if (setting_yaml.IsSequence()) {
      for (std::size_t i{0}; i < setting_yaml.size(); i++)
        Compiler::extract(setting_yaml[i], settings);
    } else {
      Compiler::extract(setting_yaml, settings);
    }
  }
}

std::map<int, std::map<int, uint8_t>> Compiler::compile(
    const std::vector<std::string>& setting_files, bool prepend_defaults) {
  std::map<std::string, std::map<std::string, int>> settings;
  // if we prepend the defaults, put all settings and their defaults
  // into the settings map before extraction
  if (prepend_defaults) {
    settings = defaults();
  }
  extract(setting_files, settings);
  return compile(settings);
}

std::map<int, std::map<int, uint8_t>> Compiler::compile(
    const std::string& setting_file, bool prepend_defaults) {
  return compile(std::vector<std::string>{setting_file}, prepend_defaults);
}

void Compiler::extract(
    YAML::Node params,
    std::map<std::string, std::map<std::string, int>>& settings) {
  // deduce list of page names for search
  //    only do this once per program run
  static std::vector<std::string> page_names;
  if (page_names.empty()) {
    for (auto& page : parameter_lut_) page_names.push_back(page.first);
  }

  if (params.IsNull()) {
    // skip null nodes (probably comments)
    return;
  }

  if (not params.IsMap()) {
    PFEXCEPTION_RAISE("BadFormat", "The YAML node provided is not a map.");
  }

  for (const auto& page_pair : params) {
    std::string page_name = page_pair.first.as<std::string>();
    YAML::Node page_settings = page_pair.second;
    if (not page_settings.IsMap()) {
      PFEXCEPTION_RAISE("BadFormat", "The YAML node for a page " + page_name +
                                         " is not a map.");
    }

    // apply these settings only to pages the input name
    std::vector<std::string> matching_pages;
    // determine matching function depending on format of input page
    //  if input page contains glob character '*', then match prefix,
    //  otherwise match entire word
    if (page_name.find('*') == std::string::npos) {
      std::string PAGE_NAME = upper_cp(page_name);
      std::copy_if(page_names.begin(), page_names.end(),
                   std::back_inserter(matching_pages),
                   [&](const std::string& page) { return PAGE_NAME == page; });
    } else {
      page_name = page_name.substr(0, page_name.find('*'));
      std::copy_if(page_names.begin(), page_names.end(),
                   std::back_inserter(matching_pages),
                   [&](const std::string& page) {
                     return strncasecmp(page.c_str(), page_name.c_str(),
                                        page_name.size()) == 0;
                   });
    }

    if (matching_pages.empty()) {
      PFEXCEPTION_RAISE("NotFound",
                        "The page " + page_name +
                            " does not match any pages in the look up table.");
    }

    for (const auto& page : matching_pages) {
      for (const auto& param : page_settings) {
	std::string sval;
	if (param.second.IsScalar()) {
	  try {
	    // try to parse as string first
	    sval = param.second.as<std::string>();
	  } catch (const YAML::TypedBadConversion<std::string>&) {
	    try {
	      // fallback: parse as int and convert to string
	      int ival = param.second.as<int>();
	      sval = std::to_string(ival);
	    } catch (const YAML::TypedBadConversion<int>&) {
	      PFEXCEPTION_RAISE("BadFormat", "Value for parameter " +
				param.first.as<std::string>() + " is neither string nor int.");
	    }
	  }
	} else {
	  PFEXCEPTION_RAISE("BadFormat", "Non-scalar value for parameter " +
			    param.first.as<std::string>());
	}
	
        if (sval.empty()) {
          PFEXCEPTION_RAISE("BadFormat", "Non-existent value for parameter " +
                                             param.first.as<std::string>());
        }
        std::string param_name = upper_cp(param.first.as<std::string>());
        settings[page][param_name] = utility::str_to_int(sval);
      }
    }
  }
}

}  // namespace pflib
