#ifndef PFLIB_COMPILE_COMPILER_H
#define PFLIB_COMPILE_COMPILER_H

#include <yaml-cpp/yaml.h>

namespace pflib::compile {

/**
 * Compiling which translates parameter values for the HGCROC
 * into register values that can be written to the chip
 *
 * ret_map[page_number][register_number] == register_value
 */
std::map<int,std::map<int,uint8_t>>
compile(const std::map<std::string,std::map<std::string,int>>& settings);

/**
 * unpack register values into the page names,
 * parameter names and parameter values mapping
 *
 * ret_map[page_name][parameter_name] == parameter_value
 */
std::map<std::string,std::map<std::string,int>>
decompile(const std::map<int,std::map<int,uint8_t>>& compiled_config);

namespace detail {
/**
 * apply the input YAML node onto the input settings map
 */
void apply(YAML::Node params,
    std::map<std::string,std::map<std::string,int>>& settings);
}

/**
 * Compile a series of yaml files
 */
std::map<int,std::map<int,uint8_t>>
compile(const std::vector<std::string>& setting_files, bool prepend_defaults = true);

/// short hand for a single setting file
std::map<int,std::map<int,uint8_t>>
compile(const std::string& setting_file, bool prepend_defaults = true) {
  return compile(std::vector<std::string>{setting_file}, prepend_defaults);
}

}

#endif
