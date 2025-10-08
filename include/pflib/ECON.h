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

  void setRunMode(bool active = true);
  bool isRunMode();

  std::vector<uint8_t> getValues(int reg_addr, int nbytes);
  void setValue(int reg_addr, uint64_t value, int nbytes);
  void setValues(int reg_addr, const std::vector<uint8_t>& values);

  void setRegisters(const std::map<int, std::map<int, uint8_t>>& registers);
  void loadParameters(const std::string& file_path, bool prepend_defaults);
  
 private:
  I2C& i2c_;
  uint8_t econ_base_;
  Compiler compiler_;
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get("econ")};
};

}  // namespace pflib

#endif  // PFLIB_ECON_H_INCLUDED
