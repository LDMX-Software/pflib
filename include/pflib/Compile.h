/**
 * @file Compile.h
 * Definition of compiling and decompiling functions
 * between page numbers, register numbers, and register values
 * to and from page names, parameter names, and parameter values.
 */
#ifndef PFLIB_COMPILE_H
#define PFLIB_COMPILE_H

#include <map>
#include <string>
#include <vector>

namespace pflib {

/**
 * Get an integer from the input string
 *
 * The normal stoi (and similar) tools don't support binary inputs
 * which are helpful in our case where sometimes the value is set
 * in binary but each bit has a non-base-2 scale.
 *
 * Supported prefixes:
 *  0b --> binary
 *  0x --> hexidecimal
 *  0  --> octal
 *  none of the above --> decimal
 */
int str_to_int(std::string str);

/**
 * Overlay a single parameter onto the input register map.
 * This only overwrites the bits that need to be changed
 * for the input parameter. Any registers that don't exist
 * will be set to 0 before being written to.
 *
 * implementation of compiling a single value for the input parameter specification
 * into the list of registers.
 *
 * This accesses the PARAMETER_LUT map, its submaps, and the register_values map without
 * any checks so it may throw a std::out_of_range error. Do checking of names
 * before calling this one.
 *
 * **Most** of the parameters have the same names as the ones in the manual;
 * however, a few parameters on the Top sub-block (page) different in format
 * and so we have changed them here. The translations from the manual to what
 * is used here are listed below.
 * - DIV_PLL_{A,B} -> DIV_PLL (two bit field)
 * - EN{1,2,3}_probe_pll -> PLL_probe_amplitude (three bit field)
 * - EN-pE{0,1,2}_probe_pll -> PLL_probe_pre_scale (three bit field)
 * - S{0,1}_probe_pll -> PLL_probe_pre_phase (two bit field)
 * - EN{1,2,3} -> eT_amplitude (three bit field)
 * - EN-pE{0,1,2} -> eT_pre_scale (three bit field)
 * - S{0,1} -> eT_pre_phase (two bit field)
 */
void compile(const std::string& page, const std::string& param, const int& val,
    std::map<int,std::map<int,uint8_t>>& registers);

/**
 * Compile a single parameter into the (potentially several)
 * registers that it should set. Any other bits in the register(s)
 * that this parameter affects will be set to zero.
 */
std::map<int,std::map<int,uint8_t>>
compile(const std::string& page_name, const std::string& param_name, const int& val);

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
 * Extract the page name, parameter name, and parameter values from
 * the YAML files into the passed settings map
 */
void extract(const std::vector<std::string>& setting_files, 
    std::map<std::string,std::map<std::string,int>>& settings);
 
/**
 * compile a series of yaml files
 */
std::map<int,std::map<int,uint8_t>>
compile(const std::vector<std::string>& setting_files, bool prepend_defaults = true);

/**
 * short hand for a single setting file
 */
std::map<int,std::map<int,uint8_t>>
compile(const std::string& setting_file, bool prepend_defaults = true);

}

#endif
