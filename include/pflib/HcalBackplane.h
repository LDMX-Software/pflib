#ifndef pflib_hcal_inc_
#define pflib_hcal_inc_

#include <memory>

#include "pflib/Bias.h"
#include "pflib/GPIO.h"
#include "pflib/Target.h"

namespace pflib {

/**
 * representing an HcalBackplane
 */
class HcalBackplane : public Target {
 public:
  virtual ~HcalBackplane() = default;
  HcalBackplane();

  /** number of boards */
  virtual int nrocs() override { return nhgcroc_; }

  /// number of econds
  virtual int necons() override { return necon_; }

  /** do we have a roc with this id? */
  virtual bool have_roc(int iroc) const override;

  /** do we have an econ with this id? */
  virtual bool have_econ(int iecon) const override;

  /** get a list of the IDs we have set up */
  virtual std::vector<int> roc_ids() const override;

  /** get a list of the econ IDs we have set up */
  virtual std::vector<int> econ_ids() const override;

  /** Get a ROC interface for the given HGCROC board */
  virtual ROC& roc(int which) override;

  /** get a ECON interface for the given econ board */
  virtual ECON& econ(int which) override;

  /** Get an I2C interface for the given HGCROC board's bias bus  */
  Bias bias(int which);

  /** Get the GPIO object for debugging purposes */
  virtual GPIO& gpio() { return *gpio_; }

  /** get the Elinks object */
  virtual Elinks& elinks() = 0;

  /** get the DAQ object */
  virtual DAQ& daq() = 0;

  /** get the lpGBT DAQ object */
  virtual lpGBT& daq_lpgbt() = 0;

  /** get the lpGBT TRG object */
  virtual lpGBT& trig_lpgbt() = 0;

 protected:
  /** Add a ROC to the set of ROCs */
  void add_roc(int iroc, uint8_t roc_baseaddr,
               const std::string& roc_type_version,
               std::shared_ptr<I2C> roc_i2c, std::shared_ptr<I2C> bias_i2c,
               std::shared_ptr<I2C> board_i2c);

  /** Add a ECON to the set of ECONs */
  void add_econ(int iecon, uint8_t econ_baseaddr, const std::string& econ_type,
                std::shared_ptr<I2C> econ_i2c);

  /** Number of HGCROC boards in this system */
  int nhgcroc_;

  /** Number of ECON boards in this system */
  int necon_;

  /** The GPIO interface */
  std::unique_ptr<GPIO> gpio_;

  /** The I2C interfaces and objects for a HGCROC board */
  struct ROCConnection {
    pflib::ROC roc_;
    std::shared_ptr<I2C> roc_i2c_;
    pflib::Bias bias_;
    std::shared_ptr<I2C> bias_i2c_;
    std::shared_ptr<I2C> board_i2c_;
  };
  std::map<int, ROCConnection> roc_connections_;

  /** The I2C interface and object for a ECON board */
  struct ECONConnection {
    pflib::ECON econ_;
    std::shared_ptr<I2C> i2c_;
  };
  std::map<int, ECONConnection> econ_connections_;
};

}  // namespace pflib

#endif  // pflib_hcal_inc_
