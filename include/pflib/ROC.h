#ifndef PFLIB_ROC_H_INCLUDED
#define PFLIB_ROC_H_INCLUDED

#include "pflib/I2C.h"
#include <vector>

namespace pflib {

  /**
   * @class ROC setup
   *
   */
class ROC {
 public:
  ROC(I2C& i2c, int ibus=0);

  std::vector<uint8_t> readPage(int ipage, int len);
  void setValue(int page, int offset, uint32_t value);
  
  std::vector<uint8_t> getChannelParameters(int ichan);
  void setChannelParameters(int ichan, std::vector<uint8_t>& values);


 private:
  I2C& i2c_;
  int ibus_;
};      
  
}

#endif // PFLIB_ROC_H_INCLUDED
