#ifndef pflib_hcal_inc_
#define pflib_hcal_inc_

#include "pflib/ROC.h"
#include "pflib/I2C.h"
#include "pflib/Bias.h"
#include "pflib/Elinks.h"
#include "pflib/GPIO.h"
#include "pflib/FastControl.h"
#include "pflib/DAQ.h"

namespace pflib {
/**
 * Class representing a standard HCAL motherboard
 */
class Hcal {
 public:
  Hcal(WishboneInterface* wb);

  /** Get a ROC interface for the given HGCROC board */
  ROC roc(int which);

  /** Get an I2C interface for the given HGCROC board's bias bus  */
  Bias bias(int which);

  /** Generate a hard reset to all the HGCROC boards */
  void hardResetROCs();

  /** Get the firmware version */
  uint32_t getFirmwareVersion();

  /** Generate a soft reset to a specific HGCROC board, -1 for all */
  void softResetROC(int which=-1);

  /** Generate a resyncLoad to a specific HGCROC board, -1 for all */
  void resyncLoadROC(int which=-1);

  /** get the Elinks object */
  Elinks& elinks() { return elinks_; }
  
  /** get the FastControl object */
  FastControl& fc() { return fc_; }

  /** get the DAQ object */
  DAQ& daq() { return daq_; }
  
 private:
  /** The wishbone interface */
  WishboneInterface* wb_;
  
  /** The GPIO interface */
  GPIO gpio_;
  
  /** The I2C interface */
  I2C i2c_;

  /** The Elinks interface */
  Elinks elinks_;
  
  /** The FastControl interface */
  FastControl fc_;

  /** The DAQ interface */
  DAQ daq_;

};

}


#endif // pflib_hcal_inc_
