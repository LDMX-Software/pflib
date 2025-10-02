#ifndef pflib_hcal_inc_
#define pflib_hcal_inc_

#include <memory>

#include "pflib/Bias.h"
#include "pflib/Elinks.h"
#include "pflib/GPIO.h"
#include "pflib/I2C.h"
#include "pflib/ROC.h"
#include "pflib/ECON.h"
// #include "pflib/FastControl.h"
#include "pflib/DAQ.h"

namespace pflib {

/**
 * representing a standard HCAL backplane or a test system
 */
class Hcal {
 public:
  Hcal();

  /** number of boards */
  int nrocs() { return nhgcroc_; }

  int necons() { return necon_; }

  /** do we have a roc with this id? */
  bool have_roc(int iroc) { return i2c_for_rocbd_.find(iroc)!=i2c_for_rocbd_.end(); }

  bool have_econ(int iecon) { return i2c_for_econbd_.find(iecon)!=i2c_for_econbd_.end(); }

  /** Get a ROC interface for the given HGCROC board */
  ROC roc(int which, const std::string& roc_type_version = "sipm_rocv3b");

  /** Get a ECON interface */
  ECON econ(int which, const std::string& type_econ = "econd");

  /** Get an I2C interface for the given HGCROC board's bias bus  */
  Bias bias(int which);

  /** Generate a hard reset to all the HGCROC boards */
  virtual void hardResetROCs();

  /** Get the firmware version */
  uint32_t getFirmwareVersion();

  /** Generate a soft reset to a specific HGCROC board, -1 for all */
  virtual void softResetROC(int which = -1);

  virtual void hardResetECONs();
  virtual void softResetECONs();

  /** Get the GPIO object for debugging purposes */
  virtual GPIO& gpio() { return *gpio_; }

  /** get the Elinks object */
  virtual Elinks& elinks() = 0;

  /** get the FastControl object */
  //  FastControl& fc() { return fc_; }

  /** get the DAQ object */
  virtual DAQ& daq() = 0;

  void print_i2c_map() {
    for (const auto& [key, ptr] : i2c_for_econbd_) {
      std::cout << "Hcal ECON I2C Key: " << key << ", I2C pointer: " << ptr.get() << std::endl;
    }
  }
  
 protected:
  /** Add a ROC to the set of ROCs */
  void add_roc(int iroc, std::shared_ptr<I2C>& roc_i2c, std::shared_ptr<I2C>& bias_i2c, std::shared_ptr<I2C>& board_i2c);

  /** Add an ECON to the set of ECONs */
  void add_econ(int iecon, std::shared_ptr<I2C>& econ_i2c);
  
  /** Number of HGCROC boards in this system */
  int nhgcroc_;

  int necon_;

  /** The GPIO interface */
  std::unique_ptr<GPIO> gpio_;

  /** The ROC I2C interfaces */
  struct I2C_per_ROC {
    std::shared_ptr<I2C> roc_i2c_;
    std::shared_ptr<I2C> bias_i2c_;
    std::shared_ptr<I2C> board_i2c_;
  };
  
  std::map<int, I2C_per_ROC> i2c_for_rocbd_;

  std::map<int, std::shared_ptr<I2C>> i2c_for_econbd_;

};

}  // namespace pflib

#endif  // pflib_hcal_inc_
