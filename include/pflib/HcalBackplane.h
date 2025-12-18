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
  /// virtual destructor since we'll be holding this as a Target
  virtual ~HcalBackplane() = default;

  HcalBackplane();

  /**
   * Common initialization for slow control given lpGBT
   * objects and a mask for which HGCROC boards are connected
   *
   * @param[in] daq_lpgbt accessor to DAQ lpGBT
   * @param[in] trig_lpgbt accessor to TRIG lpGBT
   * @param[in] hgcroc_boards bit-mask saying if a board is active/enabled (1)
   * or inactive/disabled (0) - the bit in position i represents board i
   */
  void init(lpGBT& daq_lpgbt, lpGBT& trig_lpgbt, int hgcroc_boards);

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

  /** Get the ROC to eRx mapping */
  const std::vector<std::pair<int, int>>& getRocErxMapping() override;

 protected:
  /** Number of HGCROC boards in this system */
  int nhgcroc_;

  /** Number of ECON boards in this system */
  int necon_;

  /** The GPIO interface */
  std::unique_ptr<GPIO> gpio_;

  /**
   * an HGCROC board contains one ROC, one Bias board
   * and some other utilities that are accessible
   * via the Bias C++ object
   */
  struct HGCROCBoard {
    ROC roc;
    Bias bias;
  };

  /// the backplane can hold up to 4 HGCROC boards
  std::array<std::unique_ptr<HGCROCBoard>, 4> rocs_;

  /// the ECONs on the ECON Mezzanine on this backplane
  std::array<std::unique_ptr<ECON>, 3> econs_;
};

}  // namespace pflib

#endif  // pflib_hcal_inc_
