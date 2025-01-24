#ifndef pflib_hcal_inc_
#define pflib_hcal_inc_

#include <memory>
#include "pflib/ROC.h"
#include "pflib/I2C.h"
#include "pflib/Bias.h"
//#include "pflib/Elinks.h"
#include "pflib/GPIO.h"
//#include "pflib/FastControl.h"
//#include "pflib/DAQ.h"

namespace pflib {

/**
 * representing a standard HCAL motherboard or a test system
 */
class Hcal {
 public:
  Hcal(const std::vector<std::shared_ptr<I2C>>& roc_i2c);

  /** Get a ROC interface for the given HGCROC board */
  ROC roc(int which);

  /** Get an I2C interface for the given HGCROC board's bias bus  */
  //  Bias bias(int which);

  /** Generate a hard reset to all the HGCROC boards */
  virtual void hardResetROCs();

  /** Get the firmware version */
  uint32_t getFirmwareVersion();

  /** Generate a soft reset to a specific HGCROC board, -1 for all */
  virtual void softResetROC(int which=-1);

  /** Get the GPIO object for debugging purposes */
  virtual GPIO& gpio() { return *gpio_; }
  
  /** get the Elinks object */
  //  Elinks& elinks() { return elinks_; }
  
  /** get the FastControl object */
  //  FastControl& fc() { return fc_; }

  /** get the DAQ object */
  //  DAQ& daq();
  
 protected:
  /** Number of HGCROC boards in this system */
  int nhgcroc_;
  
  /** The GPIO interface */
  std::unique_ptr<GPIO> gpio_;
  
  /** The ROC I2C interfaces */
  std::vector<std::shared_ptr<I2C>> roc_i2c_;

  /** The Elinks interface */
  //  Elinks elinks_;
  
  /** The FastControl interface */
  //  FastControl fc_;

  /** The DAQ interface */
  //  DAQ daq_;

};

}


#endif // pflib_hcal_inc_
