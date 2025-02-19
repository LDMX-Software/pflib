#ifndef PFLIB_ROC_H_INCLUDED
#define PFLIB_ROC_H_INCLUDED

#include "pflib/I2C.h"
#include "pflib/Compile.h"
#include <vector>
#include <map>
#include <string>

namespace pflib {

/**
 * @class ROC setup
 *
 */
class ROC {
 public:
  ROC(I2C& i2c, uint8_t roc_base_addr, const std::string& type_version);

  void setRunMode(bool active=true);
  bool isRunMode();
  
  std::vector<uint8_t> readPage(int ipage, int len);
  uint8_t getValue(int page, int offset);
  void setValue(int page, int offset, uint8_t value);

  std::vector<std::string> getDirectAccessParameters();
  bool getDirectAccess(const std::string& name);
  bool getDirectAccess(int reg, int bit);
  void setDirectAccess(const std::string& name, bool val);
  void setDirectAccess(int reg, int bit, bool val);
  
  std::vector<uint8_t> getChannelParameters(int ichan);
  void setChannelParameters(int ichan, std::vector<uint8_t>& values);

  void setRegisters(const std::map<int,std::map<int,uint8_t>>& registers);
  std::vector<std::string> parameters(const std::string& page);
  std::map<std::string,std::map<std::string,int>> defaults();

  void applyParameters(const std::map<std::string,std::map<std::string,int>>& parameters);

  // short-hand for just applying a single parameter
  void applyParameter(const std::string& page, const std::string& param, const int& val);


 private:
  I2C& i2c_;
  uint8_t roc_base_;
  Compiler compiler_;
};      
  
}

#endif // PFLIB_ROC_H_INCLUDED
