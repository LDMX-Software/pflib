#ifndef PFLIB_COMPILE_H
#define PFLIB_COMPILE_H

#include <map>
#include <string>
#include <vector>

namespace pflib {

/**
 * Compiling which translates parameter values for the HGCROC
 * into register values that can be written to the chip
 *
 * ret_map[page_number][register_number] == register_value
 *
 * The input settings map has an expected structure:
 *  key1: name of page (e.g. Channel_0, or Digital_Half_1)
 *  key2: name of parameter (e.g. Inputdac)
 *  val: value of parameter
 *
 * The pages that a specific parameter and its value are applied
 * to are decided by if key1 is a substring of the full page name.
 * This allows the user to specify a group of pages by just giving
 * a partial page name that is a substring of the pages that you
 * wish to apply the parameter to. For example,
 *
 *  Channel_ - matches all 71 channels pages
 *  CALIB    - matches both calib channel pages
 *  Global_Analog - matches both global analog pages
 */
std::map<int,std::map<int,uint8_t>>
compile(const std::map<std::string,std::map<std::string,int>>& settings);

/**
 * unpack register values into the page names,
 * parameter names and parameter values mapping
 *
 * ret_map[page_name][parameter_name] == parameter_value
 *
 * The decompilation simply interates through **all** 
 * parameters and attempts to deduce their values from
 * the input compiled config. If any of the registers 
 * required to deduce a parameter is missing, warnings
 * are printed to std::cerr and the parameters is skipped.
 */
std::map<std::string,std::map<std::string,int>>
decompile(const std::map<int,std::map<int,uint8_t>>& compiled_config);

 
/**
 * Compile a series of yaml files
 */
std::map<int,std::map<int,uint8_t>>
compile(const std::vector<std::string>& setting_files, bool prepend_defaults = true);

/// short hand for a single setting file
std::map<int,std::map<int,uint8_t>>
compile(const std::string& setting_file, bool prepend_defaults = true);

}

#endif
