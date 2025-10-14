#include "pflib/Compile.h"

#include <strings.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cinttypes>
#include <iostream>
#include <map>
#include <numeric>

#include "pflib/Exception.h"

namespace pflib {

std::string upper_cp(const std::string& str) {
  std::string STR{str};
  for (auto& c : STR) c = toupper(c);
  return STR;
}

#include "register_maps/register_maps.h"

Compiler Compiler::get(const std::string& type_version) {
  auto chip_it = REGISTER_MAP_BY_TYPE.find(type_version);
  if (chip_it != REGISTER_MAP_BY_TYPE.end()) {
    // printf("[DEBUG] Found type: %s\n", type_version.c_str());

    // print out the first few page names
    /*
    const auto& page_lut = chip_it->second.first;
    printf("[DEBUG] PAGE_LUT addr: %p, size=%zu\n",
           (void*)&page_lut, page_lut.size());
    if (!page_lut.empty()) {
      printf("[DEBUG] First few pages:\n");
      int count = 0;
      for (const auto& p : page_lut) {
        printf("    %s\n", p.first.c_str());
        if (++count > 5) break;
      }
    } else {
      printf("[WARN] PAGE_LUT is empty!\n");
    }
    */

    return Compiler(chip_it->second.second, chip_it->second.first);
  }

  PFEXCEPTION_RAISE("BadType",
                    "Type_version " + type_version +
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
                       const std::string& param_name, const uint64_t& val,
                       std::map<int, std::map<int, uint8_t>>& register_values) {
  const auto& page_id{parameter_lut_.at(page_name).first};
  const Parameter& spec{parameter_lut_.at(page_name).second.at(param_name)};
  uint64_t uval{static_cast<uint64_t>(val)};

  std::size_t total_nbits =
      std::accumulate(spec.registers.begin(), spec.registers.end(), 0,
                      [](std::size_t bit_count, const RegisterLocation& rhs) {
                        return bit_count + rhs.n_bits;
                      });
  std::size_t val_msb = msb(uval);

  if (val_msb >= total_nbits) {
    std::stringstream msg;
    msg << "Parameter " << page_name << '.' << param_name
        << " is being set to a value (" << val << ") exceeding its size ("
        << total_nbits << " bits -> value < " << (1u << total_nbits) << ")";
    PFEXCEPTION_RAISE("ValOver", msg.str());
  }

  std::size_t value_curr_min_bit{0};
  pflib_log(trace) << page_name << "." << param_name << " -> page " << page_id;
  for (const RegisterLocation& location : spec.registers) {
    // grab sub value of parameter in this register
    uint8_t sub_val = ((uval >> value_curr_min_bit) & location.mask);
    pflib_log(trace) << "  " << sub_val << " at " << location.reg << ", "
                     << location.n_bits << " bits";
    value_curr_min_bit += location.n_bits;
    if (register_values[page_id].find(location.reg) ==
        register_values[page_id].end()) {
      // initialize register value to zero if it hasn't been touched before
      register_values[page_id][location.reg] = 0;
    } else {
      // make sure to clear old value
      register_values[page_id][location.reg] &=
          ((location.mask << location.min_bit) ^ 0b11111111);
    }
    // put value into register at the specified location
    register_values[page_id][location.reg] += (sub_val << location.min_bit);

  }  // loop over register locations
  return;
}

std::map<int, std::map<int, uint8_t>> Compiler::compile(
    const std::string& page_name, const std::string& param_name,
    const uint64_t& val) {
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
    const std::map<std::string, std::map<std::string, uint64_t>>& settings) {
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

std::map<uint16_t, size_t> Compiler::build_register_byte_lut() {
  // register address -> number of bytes used
  std::map<uint16_t, size_t> reg_byte_lut;

  for (const auto& page_pair : page_lut_) {
    const std::string& page_name = page_pair.first;
    const Page& page = page_pair.second;

    std::set<uint16_t> all_used_regs;

    for (const auto& param_pair : page) {
      const Parameter& param = param_pair.second;
      std::vector<RegisterLocation> regs = param.registers;

      // start and end of the first (regs[0]) contiguous block of addresses for
      // this parameter
      uint16_t start = regs[0].reg;
      uint16_t end = start + (regs[0].min_bit + regs[0].n_bits + 7) / 8 - 1;

      // iterate over remaining register locations for this parameter
      for (size_t i = 1; i < regs.size(); ++i) {
        uint16_t loc_start = regs[i].reg;
        uint16_t loc_end =
            loc_start + (regs[i].min_bit + regs[i].n_bits + 7) / 8 - 1;

        if (loc_start <= end + 1) {
          // overlapping or contiguous with previous block: extend the block
          end = std::max(end, loc_end);
        } else {
          // non-contiguous: save previous block if it does not exist (prevents
          // overwriting)...
          if (reg_byte_lut.find(start) == reg_byte_lut.end()) {
            reg_byte_lut[start] = end - start + 1;
          }

          // start a new one
          start = loc_start;
          end = loc_end;
        }
      }

      // write the last block
      // e.g. if a previous parameter already wrote a block starting at addr_x
      // of length 1, but the current parameter extends to include addr_x+1, the
      // map will update to length=2
      uint16_t block_len = end - start + 1;
      auto it = reg_byte_lut.find(start);
      if (it == reg_byte_lut.end()) {
        reg_byte_lut[start] = block_len;
      } else {
        it->second =
            std::max(it->second, static_cast<decltype(it->second)>(block_len));
      }
    }
  }

  return reg_byte_lut;
}

std::vector<int> Compiler::get_known_pages() {
  std::vector<int> known_pages;
  known_pages.reserve(parameter_lut_.size());
  for (const auto& [page_name, page_spec] : parameter_lut_) {
    known_pages.push_back(page_spec.first);
  }
  return known_pages;
}

std::map<std::string, std::map<std::string, uint64_t>> Compiler::decompile(
    const std::map<int, std::map<int, uint8_t>>& compiled_config,
    bool be_careful, bool little_endian) {
  std::map<std::string, std::map<std::string, uint64_t>> settings;
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
    const auto& page_conf = compiled_config.at(page_id);

    // loop over each parameter
    for (const auto& param : page_lut) {
      const Parameter& spec{page_lut.at(param.first)};
      uint64_t pval{0};
      std::size_t value_curr_min_bit = 0;
      int n_missing_regs{0};

      if (little_endian) {
        // collect all relevant registers in a vector in descending order
        std::vector<uint8_t> data;
        uint16_t first_reg = spec.registers.front().reg;
        uint16_t last_reg = spec.registers.front().reg;
        for (const RegisterLocation& loc : spec.registers) {
          if (page_conf.find(loc.reg) == page_conf.end()) {
            n_missing_regs++;
            if (be_careful)
              break;
            else
              continue;
          }

          if (loc.reg < first_reg) first_reg = loc.reg;

          // compute how many bytes this field spans to set the last_reg
          uint16_t span_bytes =
              (loc.min_bit + loc.n_bits + 7) / 8;  // ceiling division
          uint16_t reg_end = loc.reg + span_bytes - 1;
          if (reg_end > last_reg) last_reg = reg_end;
        }

        if (n_missing_regs > 0 && !be_careful) {
          // pflib_log(debug) << "Skipping parameter " << param.first
          //<< " (no registers found in compiled config)";
          continue;
        }

        for (uint16_t reg = first_reg; reg <= last_reg; ++reg) {
          auto it = page_conf.find(reg);
          if (it != page_conf.end()) {
            data.push_back(it->second);
            // pflib_log(debug) << "[DEBUG] Register 0x" << std::hex << reg
            //<< ": byte=0x" << int(it->second);
          } else {
            // pflib_log(warn) << "[WARN] Missing register 0x" << std::hex <<
            // reg
            //<< " for parameter " << param.first;
            n_missing_regs++;
            data.push_back(0);  // assume 0 if missing
          }
        }

        // combine into a little endian integer
        uint64_t value = 0;
        for (size_t i = 0; i < data.size(); ++i)
          value |= (static_cast<uint64_t>(data[i]) << (8 * i));

        // pflib_log(debug) << "[DEBUG] data contents for parameter " <<
        // param.first << ":"; for (size_t i = 0; i < data.size(); ++i) {
        // pflib_log(debug) << "  data[" << i << "] = 0x" << std::hex <<
        // int(data[i]);
        // }
        // pflib_log(debug) << "value " << std::hex << value;

        for (const RegisterLocation& loc : spec.registers) {
          // extract field from this register
          uint64_t field_value = (value >> loc.min_bit) & loc.mask;

          /*
          pflib_log(debug) << "[DEBUG] Extracting field from RegisterLocation:
          reg=0x"
                          << std::hex << loc.reg
                          << ", min_bit=" << std::dec << loc.min_bit
                          << ", n_bits=" << loc.n_bits
                          << ", mask=0x" << std::hex << loc.mask
                          << ", field_value=0x" << std::hex << field_value;
          */
          pval |= field_value;
        }

        // pflib_log(debug) << "[DEBUG] Parameter '" << param.first
        //<< "' final value = 0x" << std::hex << pval;

      } else {
        // non-little-endian logic
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
      }

      if (n_missing_regs == spec.registers.size() or
          (be_careful and n_missing_regs > 0)) {
        // skip this parameter
        if (be_careful) {
          pflib_log(warn)
              << "parameter " << param.first << " in page " << page_name
              << " wasn't provided the necessary registers to be deduced";

          std::ostringstream oss;
          oss << "  Expected registers: ";
          for (const auto& loc : spec.registers) {
            oss << "0x" << std::hex << loc.reg << " ";
          }
          pflib_log(warn) << oss.str();

          std::ostringstream present;
          present << "  Registers provided in compiled_config[" << page_name
                  << "]: ";
          for (const auto& kv : page_conf) {
            present << "0x" << std::hex << kv.first << " ";
          }
          pflib_log(warn) << present.str();
        }
      } else {
        settings[page_name][param.first] = pval;
        pflib_log(debug) << "Parameter '" << param.first << "' final value = 0x"
                         << std::hex << pval << " (" << std::dec << pval << ")";
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

std::map<std::string, std::map<std::string, uint64_t>> Compiler::defaults() {
  std::map<std::string, std::map<std::string, uint64_t>> settings;
  for (auto& page : parameter_lut_) {
    for (auto& param : page.second.second) {
      settings[page.first][param.first] = param.second.def;
    }
  }
  return settings;
}

void Compiler::extract(
    const std::vector<std::string>& setting_files,
    std::map<std::string, std::map<std::string, uint64_t>>& settings) {
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
  std::map<std::string, std::map<std::string, uint64_t>> settings;
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
    std::map<std::string, std::map<std::string, uint64_t>>& settings) {
  // deduce list of page names for search
  //    only do this once per program run
  // static std::vector<std::string> page_names;
  // Commented this out for now .. otherwise if we run the econd and roc tests
  // at the same time it will fail.. FIX by separating them
  std::vector<std::string> page_names;
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
      // pflib_log(debug)
      //<< "[DEBUG] Searching for wildcard match. Page_Name: '" << page_name <<
      //"'";
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
                                                 param.first.as<std::string>() +
                                                 " is neither string nor int.");
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
        settings[page][param_name] =
            std::stoull(sval, nullptr, 0);  // base 0 allows hex
        // settings[page][param_name] = utility::str_to_int(sval);
      }
    }
  }
}

}  // namespace pflib
