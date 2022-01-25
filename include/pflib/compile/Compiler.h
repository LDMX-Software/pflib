#ifndef PFLIB_COMPILE_COMPILER_H
#define PFLIB_COMPILE_COMPILER_H

#include <yaml-cpp/yaml.h>

namespace pflib::compile {

/**
 * "Compiler" which translates parameter values for the HGCROC
 * into register values that can be written to the chip
 */
class Compiler {
  /// map of names to settings
  ///   settings_[page_name][param_name] == param_val
  std::map<std::string,std::map<std::string,int>> settings_;
  /// apply the input YAML node to the settings map
  void apply(YAML::Node params);
 public:
  Compiler(const std::vector<std::string>& setting_files, bool prepend_defaults = true);
  /// short hand for a single setting file
  Compiler(const std::string& setting_file, bool prepend_defaults = true) 
    : Compiler(std::vector<std::string>{setting_file}, prepend_defaults) {}
  /**
   * get the register values from the settings
   * 
   * ret_map[page_number][register_number] == register_value
   */
  std::map<int,std::map<int,uint8_t>> compile();
};

}

#endif
