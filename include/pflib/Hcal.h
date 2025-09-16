#ifndef pflib_hcal_inc_
#define pflib_hcal_inc_

#include <memory>

#include "pflib/Bias.h"
#include "pflib/Elinks.h"
#include "pflib/GPIO.h"
#include "pflib/I2C.h"
#include "pflib/ROC.h"
// #include "pflib/FastControl.h"
#include "pflib/DAQ.h"

namespace pflib {

/**
 * representing a standard HCAL backplane or a test system
 */
class Hcal {
 public:
  Hcal(const std::vector<std::shared_ptr<I2C>>& roc_i2c,
       const std::vector<std::shared_ptr<I2C>>& bias_i2c);

  /** number of boards */
  int nrocs() { return nhgcroc_; }

  /** Get a ROC interface for the given HGCROC board */
  ROC roc(int which, const std::string& roc_type_version = "sipm_rocv3b");

  /** Get an I2C interface for the given HGCROC board's bias bus  */
  Bias bias(int which);

  /** Generate a hard reset to all the HGCROC boards */
  virtual void hardResetROCs();

  /** Get the firmware version */
  uint32_t getFirmwareVersion();

  /** Generate a soft reset to a specific HGCROC board, -1 for all */
  virtual void softResetROC(int which = -1);

  /** Get the GPIO object for debugging purposes */
  virtual GPIO& gpio() { return *gpio_; }

  /** get the Elinks object */
  virtual Elinks& elinks() = 0;

  /** get the FastControl object */
  //  FastControl& fc() { return fc_; }

  /** get the DAQ object */
  virtual DAQ& daq() = 0;

 protected:
  /** Number of HGCROC boards in this system */
  int nhgcroc_;

  /** The GPIO interface */
  std::unique_ptr<GPIO> gpio_;

  /** The ROC I2C interfaces */
  std::vector<std::shared_ptr<I2C>> roc_i2c_;
  std::vector<std::shared_ptr<I2C>> bias_i2c_;
};

}  // namespace pflib

#endif  // pflib_hcal_inc_
