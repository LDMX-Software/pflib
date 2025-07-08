/**
 * @file Compile.h
 * Definition of compiling and decompiling functions
 * between page numbers, register numbers, and register values
 * to and from page names, parameter names, and parameter values.
 */
#ifndef PFLIB_COMPILE_H
#define PFLIB_COMPILE_H

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "pflib/Logging.h"
#include "pflib/utility/str_to_int.h"
#include "register_maps/register_maps_types.h"

namespace YAML {
class Node;
}

namespace pflib {

/**
 * Get a copy of the input string with all caps
 */
std::string upper_cp(const std::string& str);

/**
 * The object that does the compiling
 *
 * Since the compiler needs to be configured,
 * we wrap all of the compiling methods into a class
 * so that any callers must define the compiler configuration
 * before they are able to compile.
 *
 * Right now, the compiler only has one configuration parameter:
 * which type and which version of the HGCROC it should compile for.
 */
class Compiler {
 public:
  /**
   * Get an instance of the compiler for the input HGCROC type_version string
   */
  static Compiler get(const std::string& roc_type_version);

  /**
   * Overlay a single parameter onto the input register map.
   *
   * This only overwrites the bits that need to be changed
   * for the input parameter. Any registers that don't exist
   * will be set to 0 before being written to.
   *
   * implementation of compiling a single value for the input parameter
   * specification into the list of registers.
   *
   * This accesses the PARAMETER_LUT map, its submaps, and the register_values
   * map without any checks so it may throw a std::out_of_range error. Do
   * checking of names before calling this one.
   *
   * **Most** of the parameters have the same names as the ones in the
   * \hgcrocmanual; however, a few parameters on the Top sub-block (page)
   * different in format and so we have changed them here. The translations from
   * the manual to what is used here are listed below.
   * - DIV_PLL_{A,B} -> DIV_PLL (two bit field)
   * - EN{1,2,3}_probe_pll -> PLL_probe_amplitude (three bit field)
   * - EN-pE{0,1,2}_probe_pll -> PLL_probe_pre_scale (three bit field)
   * - S{0,1}_probe_pll -> PLL_probe_pre_phase (two bit field)
   * - EN{1,2,3} -> eT_amplitude (three bit field)
   * - EN-pE{0,1,2} -> eT_pre_scale (three bit field)
   * - S{0,1} -> eT_pre_phase (two bit field)
   *
   * @param[in] page name of page parameter is on
   * @param[in] param name of parameter
   * @param[in] val value parameter should be
   * @param[in,out] registers page numbers, register numbers, and register
   * values to apply parameter to
   */
  void compile(const std::string& page, const std::string& param,
               const int& val,
               std::map<int, std::map<int, uint8_t>>& registers);

  /**
   * Compile a single parameter into the (potentially several)
   * registers that it should set. Any other bits in the register(s)
   * that this parameter affects will be set to zero.
   *
   * @param[in] page_name name of page parameter is on
   * @param[in] param_name name of parameter
   * @param[in] val value parameter should be set to
   * @return page numbers, register numbers, and register values to set
   */
  std::map<int, std::map<int, uint8_t>> compile(const std::string& page_name,
                                                const std::string& param_name,
                                                const int& val);

  /**
   * Compiling which translates parameter values for the HGCROC
   * into register values that can be written to the chip
   *
   * ```
   * ret_map[page_number][register_number] == register_value
   * ```
   *
   * The input settings map has an expected structure:
   * - key1: name of page (e.g. Channel_0, or Digital_Half_1)
   * - key2: name of parameter (e.g. Inputdac)
   * - val: value of parameter
   *
   * The pages that a specific parameter and its value are applied
   * to are decided by if key1 is a substring of the full page name.
   * This allows the user to specify a group of pages by just giving
   * a partial page name that is a substring of the pages that you
   * wish to apply the parameter to. For example,
   *
   * - Channel_* - matches all 71 channels pages
   * - CALIB*    - matches both calib channel pages
   * - Global_Analog* - matches both global analog pages
   *
   * @param[in] settings page names, parameter names, and parameter value
   * settings
   * @return page numbers, register numbers, and register value settings
   */
  std::map<int, std::map<int, uint8_t>> compile(
      const std::map<std::string, std::map<std::string, int>>& settings);

  /**
   * Get the known pages from the LUTs
   *
   * This is helpful when decompiling.
   */
  std::vector<int> get_known_pages();

  /**
   * unpack register values into the page names,
   * parameter names and parameter values mapping
   *
   * ret_map[page_name][parameter_name] == parameter_value
   *
   * The decompilation simply interates through **all**
   * parameters and attempts to deduce their values from
   * the input compiled config.
   *
   * If be_careful is true and any of the registers
   * required to deduce a parameter are missing, warnings
   * are printed to std::cerr and the parameter is skipped.
   * Additionally, warnings are printed for entire pages that are skipped.
   *
   * If be_careful is false, then no warnings are printed
   * and parameters are only skipped if all of the registers
   * that constitute it are missing.
   *
   * @param[in] compiled_config page number, register number, register value
   * settings
   * @param[in] be_careful true if we should print warnings and skip
   * partially-set params
   * @return page name, parameter name, parameter value of registers provided
   */
  std::map<std::string, std::map<std::string, int>> decompile(
      const std::map<int, std::map<int, uint8_t>>& compiled_config,
      bool be_careful);

  /**
   * get the registers corresponding to the input page
   *
   * @return dummy register mapping to help with selecting
   */
  std::map<int, std::map<int, uint8_t>> getRegisters(const std::string& page);

  /**
   * get the default parameter values as specified in the manual
   *
   * This is a helpful command for simply printing a "template"
   * YAML settings file although it will printout all pages individually.
   *
   * @return map of page name and parameter names to default values
   */
  std::map<std::string, std::map<std::string, int>> defaults();

  /**
   * Extract the page name, parameter name, and parameter values from
   * the YAML files into the passed settings map
   *
   * Page names are allowed to be "globbed" so that multiple pages with the
   * same parameters can be set in one chunk of YAML. The globbing is just
   * checking for a prefix and **you must** end a globbing page with the `*`
   * character in order to signal that this chunk of YAML is to be used for
   * all pages whose name start with the input prefix.
   *
   * All matching is done case insensitively.
   *
   * ### Globbing Examples
   * ```yaml
   * channel_* :
   *   channel_off : 1
   * ```
   * will match _all_ channel pages and turn them off.
   * ```yaml
   * channel_ :
   *   channel_off : 1
   * ```
   * will throw an exception because no page is named 'channel_'.
   * ```yaml
   * channel_1 :
   *   channel_off : 1
   * ```
   * will turn off channel_1 (and _only_ channel 1)
   * ```yaml
   * channel_1* :
   *   channel_off : 1
   * ```
   * will turn off all channels beginning with 1 (1, 10, 11, 12, etc...)
   *
   * @param[in] setting_files list of YAML files to extract in order
   * @param[in,out] settings page name, parameter name, parameter value settings
   *  extracted from YAML file(s)
   */
  void extract(const std::vector<std::string>& setting_files,
               std::map<std::string, std::map<std::string, int>>& settings);

  /**
   * compile a series of yaml files
   *
   * @see extract for how the YAML files are parsed
   *
   * @param[in] setting_files list of YAML files to extract in order and compile
   * @param[in] prepend_defaults start construction of settings map by including
   *    defaults from **all** parameter settings
   * @return page numbers, register numbers, and register value settings
   */
  std::map<int, std::map<int, uint8_t>> compile(
      const std::vector<std::string>& setting_files,
      bool prepend_defaults = true);

  /**
   * short hand for a single setting file
   *
   * @see extract for how the YAML files are parsed
   *
   * @param[in] setting_file YAML file to extract and compile
   * @param[in] prepend_defaults start construction of settings map by including
   *    defaults from **all** parameter settings
   * @return page numbers, register numbers, and register value settings
   */
  std::map<int, std::map<int, uint8_t>> compile(const std::string& setting_file,
                                                bool prepend_defaults = true);

 private:
  /**
   * Private constructor, only access Compiler instances from the
   * static get method so that we can ensure they are properly configured.
   */
  Compiler(const ParameterLUT& parameter_lut, const PageLUT& page_lut);

  /**
   * Extract a map of page_name, param_name to their values by crawling the YAML
   * tree.
   *
   * @throw pflib::Exception if YAML file has a bad format (root node is not a
   * map, page nodes are not maps, page name doesn't match any pages, or
   * parameter's value does not exist).
   *
   * @param[in] params a YAML::Node to start extraction from
   * @param[in,out] settings map of names to values for extract parameters
   */
  void extract(YAML::Node params,
               std::map<std::string, std::map<std::string, int>>& settings);

 private:
  const ParameterLUT& parameter_lut_;
  const PageLUT& page_lut_;
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get("compile")};
};
}  // namespace pflib

#endif
