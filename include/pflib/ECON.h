#ifndef PFLIB_ECON_H_INCLUDED
#define PFLIB_ECON_H_INCLUDED

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "pflib/Compile.h"
#include "pflib/I2C.h"
#include "pflib/Logging.h"

namespace pflib {

/**
 * @class ECON setup
 */
class ECON {
 public:
  ECON(I2C& i2c, uint8_t econ_base_addr, const std::string& type_version);

  const std::string& type() const { return type_; }
  void setRunMode(bool active = true, int edgesel = -1, int fcmd_invert = -1);
  int getPUSMRunValue();
  int getPUSMStateValue();
  bool isRunMode();

  std::vector<uint8_t> getValues(int reg_addr, int nbytes);
  void setValue(int reg_addr, uint64_t value, int nbytes);
  void setValues(int reg_addr, const std::vector<uint8_t>& values);

  void setRegisters(const std::map<int, std::map<int, uint8_t>>& registers);
  void loadParameters(const std::string& file_path, bool prepend_defaults);
  std::map<int, std::map<int, uint8_t>> getRegisters(
      const std::map<int, std::map<int, uint8_t>>& selected);
  std::map<int, std::map<int, uint8_t>> applyParameters(
      const std::map<std::string, std::map<std::string, uint64_t>>& parameters);
  void applyParameter(const std::string& page, const std::string& param,
                      const uint64_t& val);

  std::map<std::string, std::map<std::string, uint64_t>> readParameters(
      const std::map<std::string, std::map<std::string, uint64_t>>& parameters,
      bool print_values = true);
  std::map<std::string, std::map<std::string, uint64_t>> readParameters(
      const std::string& file_path);
  void readParameter(const std::string& page, const std::string& param);
  uint64_t dumpParameter(const std::string& page, const std::string& param);

  void dumpSettings(const std::string& filename, bool should_decompile);

  class TestParameters {
    std::map<int, std::map<int, uint8_t>> previous_registers_;
    ECON& econ_;

   public:
    TestParameters(
        ECON& econ,
        std::map<std::string, std::map<std::string, uint64_t>> new_params);
    /// applies the unset parameters to the ECON
    ~TestParameters();
    /// cannot copy or assign this lock
    TestParameters(const TestParameters&) = delete;
    TestParameters& operator=(const TestParameters&) = delete;
    /// Build a TestParameters parameter by parameter
    class Builder {
      std::map<std::string, std::map<std::string, uint64_t>> parameters_;
      ECON& econ_;

     public:
      Builder(ECON& econ);
      Builder& add(const std::string& page, const std::string& param,
                   const uint64_t& val);
      [[nodiscard]] TestParameters apply();
    };
  };

  TestParameters::Builder testParameters();

 private:
  I2C& i2c_;
  uint8_t econ_base_;
  Compiler compiler_;
  std::string type_;
  std::map<uint16_t, size_t> econ_reg_nbytes_lut_;
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get("econ")};
};

}  // namespace pflib

#endif  // PFLIB_ECON_H_INCLUDED
