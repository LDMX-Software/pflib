#ifndef PFLIB_ROC_H_INCLUDED
#define PFLIB_ROC_H_INCLUDED

#include "pflib/I2C.h"
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
  ROC(I2C& i2c, uint8_t roc_base_addr);

  std::vector<uint8_t> readPage(int ipage, int len);
  void setValue(int page, int offset, uint32_t value);
  
  std::vector<uint8_t> getChannelParameters(int ichan);
  void setChannelParameters(int ichan, std::vector<uint8_t>& values);

  void setRegisters(const std::map<int,std::map<int,uint8_t>>& registers);
  void applyParameters(const std::map<std::string,std::map<std::string,int>>& parameters);

  // short-hand for just applying a single parameter
  void applyParameter(const std::string& page, const std::string& param, const int& val);


 private:
  I2C& i2c_;
  uint8_t roc_base_;
};      
  
}

#endif // PFLIB_ROC_H_INCLUDED
