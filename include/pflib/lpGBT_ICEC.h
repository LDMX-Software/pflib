#ifndef PFLIB_lpGBT_ICEC_H_INCLUDED
#define PFLIB_lpGBT_ICEC_H_INCLUDED

#include "pflib/lpGBT.h"

namespace pflib {

  /**
   * @class lpGBT configuration transport interface class partially specified
   * for IC/EC communication
   */
  class lpGBT_ICEC : public lpGBT_ConfigTransport {
   protected:
    lpGBT_ICEC(bool isEC, uint8_t lpgbt_i2c_addr);

    virtual void transact(uint16_t reg, 
  };
  
}


#endif // PFLIB_lpGBT_ICEC_H_INCLUDED


