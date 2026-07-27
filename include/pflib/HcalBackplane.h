#ifndef pflib_hcal_inc_
#define pflib_hcal_inc_

#include <memory>

#include "pflib/Bias.h"
#include "pflib/GPIO.h"
#include "pflib/TRIG.h"
#include "pflib/HcalTarget.h"

namespace pflib {

/**
 * representing an HcalBackplane
 */
class HcalBackplane : public HcalTarget {
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
   * @param[in] use_bias_cache use software cache of bias voltage settings
   * instead of readback direct from chip, see Bias class for why this
   * is necessary in fiberfull setups
   */
  void init(lpGBT& daq_lpgbt, lpGBT& trig_lpgbt, int hgcroc_boards, bool use_bias_cache);

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
  Bias& bias(int which) override;

  /** Get the GPIO object for debugging purposes */
  virtual GPIO& gpio() { return *gpio_; }

  /** get the Elinks object */
  virtual Elinks& elinks() = 0;

  /** get the DAQ object */
  virtual DAQ& daq() = 0;

  /** get the trig object, if valid */
  virtual TRIG* trig(int itrig) { return (itrig == 0) ? (trig_.get()) : (0); }

  /** Get the ROC to eRx mapping for the DAQ path */
  const std::vector<std::pair<int, int>>& getHardwareRocErxMappingDAQ()
      override;
  /** Get the ROC to eRx mapping for the TRG path*/
  const std::vector<std::pair<int, std::vector<int>>>&
  getHardwareRocErxMappingTRG() override;

  /// the ROC to eRx mapping along the DAQ path for this hardware
  static const std::vector<std::pair<int, int>> ROC_ERX_MAPPING_DAQ;
  /// the ROC to eRx mapping along the TRG path for this hardware
  static const std::vector<std::pair<int, std::vector<int>>>
      ROC_ERX_MAPPING_TRG;

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
    /// constructor to forward constructor arguments to members
    HGCROCBoard(
      std::shared_ptr<I2C> roc_i2c,
      uint8_t roc_addr,
      const std::string& roc_typename,
      std::shared_ptr<I2C> bias_i2c,
      std::shared_ptr<I2C> board_i2c,
      bool bias_use_cache
    );
  };

  /// the backplane can hold up to 4 HGCROC boards
  std::array<std::unique_ptr<HGCROCBoard>, 4> rocs_;

  /// the ECONs on the ECON Mezzanine on this backplane
  std::array<std::unique_ptr<ECON>, 3> econs_;

  /// pointer to the TRIG object if available
  std::unique_ptr<pflib::TRIG> trig_;
};

}  // namespace pflib

#endif  // pflib_hcal_inc_
