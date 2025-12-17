#ifndef PFLIB_ROC_H_INCLUDED
#define PFLIB_ROC_H_INCLUDED

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "pflib/Compile.h"
#include "pflib/I2C.h"
#include "pflib/logging/Logging.h"

namespace pflib {

/**
 * @class ROC setup
 */
class ROC {
  static const int N_REGISTERS_PER_PAGE = 32;

 public:
  ROC(std::shared_ptr<I2C> i2c, uint8_t roc_base_addr, const std::string& type_version);

  void setRunMode(bool active = true);
  bool isRunMode();
  const std::string& type() const;

  std::vector<uint8_t> readPage(int ipage, int len);
  uint8_t getValue(int page, int offset);
  void setValue(int page, int offset, uint8_t value);

  std::vector<std::string> getDirectAccessParameters();
  bool getDirectAccess(const std::string& name);
  bool getDirectAccess(int reg, int bit);
  void setDirectAccess(const std::string& name, bool val);
  void setDirectAccess(int reg, int bit, bool val);

  /**
   * set registers on the HGCROC to specific values
   *
   * @param[in] registers The input map has the page as the first key and the
   * register in the page as the second key and then the 8-bit register value.
   */
  void setRegisters(const std::map<int, std::map<int, uint8_t>>& registers);

  /**
   * get registers from the HGCROC
   *
   * @param[in] selected registers to get values for, all registers if empty
   * @return register values currently on the chip
   */
  std::map<int, std::map<int, uint8_t>> getRegisters(
      const std::map<int, std::map<int, uint8_t>>& selected = {});

  /**
   * set registers on the HGCROC to specific values
   *
   * @param[in] file_path path to CSV file containing register values,
   * the first column is the page, the second column is the register in that
   * page, and then the last column is the value of the register
   */
  void loadRegisters(const std::string& file_path);

  /**
   * Get the parameters for the input page
   *
   * @param[in] page name of page to get parameters for
   */
  std::map<std::string, uint64_t> getParameters(const std::string& page);

  /**
   * Retrieve all of the manual-documented defaults for all parameters
   *
   * The values are not very interesting (they are all zero in v3 chips),
   * but this can be helpful for looking through a parameter listing
   * organized by page.
   *
   * @return mapping of page->param->value
   */
  std::map<std::string, std::map<std::string, uint64_t>> defaults();

  /**
   * Apply the input parameter mapping onto the chip
   *
   * @param[in] parameters mapping of parameters to apply where the
   * first key is the page name, the second key is the parameter in
   * that page and the value is the parameter value
   * @return chip registers that **were** on the chip before the application
   * of these parameters, helpful if users want to unset them later with
   * ROC::TestParameters
   */
  std::map<int, std::map<int, uint8_t>> applyParameters(
      const std::map<std::string, std::map<std::string, uint64_t>>& parameters);

  /**
   * Load the input parameters onto the chip
   *
   * @param[in] file_path path to YAML file containing parameters to load
   * @param[in] prepend_defaults if true, use the default values for all
   * parameters before applying the YAML file itself, else only apply
   * the parameters contained in the YAML file
   */
  void loadParameters(const std::string& file_path, bool prepend_defaults);

  /**
   * Short-hand for applying a single parameter
   *
   * We construct a small std::map and use the applyParameters function above.
   *
   * @param[in] page name of page
   * @param[in] param name of parameter in that page
   * @param[in] val value to set parameter to
   */
  void applyParameter(const std::string& page, const std::string& param,
                      const uint64_t& val);

  /**
   * Dump the settings of the HGCROC into the input filepath
   *
   * @param[in] filepath file to which to dump settings into
   * @param[in] decompile if true, decompile the registers into parameters
   * and write a YAML file, else just write a CSV file of the registers
   */
  void dumpSettings(const std::string& filepath, bool decompile);

  /**
   * test certain parameters before setting them back to old values
   */
  class TestParameters {
    std::map<int, std::map<int, uint8_t>> previous_registers_;
    ROC& roc_;

   public:
    /**
     * Construct a set of test parameters
     *
     * Apply the input settings to the roc and store
     * the previous chip settings to be applied in the destructor
     */
    TestParameters(
        ROC& roc,
        std::map<std::string, std::map<std::string, uint64_t>> new_params);
    /// applies the unset parameters to the ROC
    ~TestParameters();
    /// cannot copy or assign this lock
    TestParameters(const TestParameters&) = delete;
    TestParameters& operator=(const TestParameters&) = delete;
    /// Build a TestParameters parameter by parameter
    class Builder {
      std::map<std::string, std::map<std::string, uint64_t>> parameters_;
      ROC& roc_;

     public:
      Builder(ROC& roc);
      Builder& add(const std::string& page, const std::string& param,
                   const uint64_t& val);
      Builder& add_all_channels(const std::string& param, const uint64_t& val);
      [[nodiscard]] TestParameters apply();
    };
  };

  /**
   * start a set of test parameters
   *
   * Use when you want to temporarily set parameters on the chip
   * which will then be re-set back to their previous values later.
   *
   * example
   * ```cpp
   * // PAGE.PARAM1 and PAGE.PARAM2 have some values from
   * // previous settings that were applied
   * {
   *   auto test_parameter_handle = roc.testParameters()
   *     .add("PAGE", "PARAM1", 42)
   *     .add("PAGE", "PARAM2", 32);
   *     .apply();
   *   // PAGE.PARAM1 == 42 and PAGE.PARAM2 == 32
   * }
   * // PAGE.PARAM1 restored to previous settings
   * ```
   */
  TestParameters::Builder testParameters();

 private:
  std::shared_ptr<I2C> i2c_;
  uint8_t roc_base_;
  std::string type_version_;
  Compiler compiler_;
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get("roc")};
};

}  // namespace pflib

#endif  // PFLIB_ROC_H_INCLUDED
