#pragma once

#include <memory>
#include <vector>

#include "pflib/ECON.h"
#include "pflib/I2C.h"
#include "pflib/ROC.h"
#include "pflib/Target.h"
#include "pflib/lpGBT.h"

namespace pflib {

/**
 * Class holding a set of ROCs and ECONs representing a HexaModule
 *
 */
class EcalModule {
 public:
  /**
   * Construct access to an EcalModule given the DAQ lpGBT
   * it talks through
   *
   * @param[in] lpgbt accessor to the lpgbt
   * @param[in] i2cbus bus on which the EcalModule is
   * @param[in] modulenumber module ID number
   * @param[in] roc_mask bit-mask saying if a board is activate/enabled (1)
   * or inactive/disabled (0) - position i represents ROC i
   */
  EcalModule(lpGBT& lpgbt, int i2cbus, int modulenumber, uint8_t roc_mask);

  /** number of hgcrocs */
  int nrocs() const;
  /// number of econs
  int necons() const;

  /** do we have a roc with this id? */
  bool have_roc(int iroc) const;

  static constexpr const int ECON_D = 0;
  static constexpr const int ECON_T = 1;

  /** do we have an econ with this id? */
  bool have_econ(int iecon) const;

  /** get a list of the IDs we have set up */
  std::vector<int> roc_ids() const;

  /** get a list of the econ IDs we have set up */
  std::vector<int> econ_ids() const;

  /** Get a ROC interface for the given HGCROC */
  ROC& roc(int which);

  /** get a ECON interface for the given ECON */
  ECON& econ(int which);

  /** Generate a hard reset to all the HGCROC boards */
  void hardResetROCs();

  /** generate a hard reset to all the ECON boards */
  void hardResetECONs();

  /** Generate a soft reset */
  void softResetROC();

  /** Generate a soft reset */
  void softResetECON();

  /** Get the mapping of ROC channels to ERX channels */
  static const std::vector<std::pair<int, int>>& getRocErxMapping();

 protected:
  lpGBT& lpGBT_;
  int i2cbus_;
  int imodule_;
  std::shared_ptr<I2C> i2c_;
  /// number of ROCs that are enabled
  int n_rocs_;
  /// up to 6 ROCs some of which could be disabled
  std::array<std::unique_ptr<ROC>, 6> rocs_;
  /// number of ECONs that are enabled
  int n_econs_;
  /// two ECONs
  std::array<std::unique_ptr<ECON>, 2> econs_;
 private:
  /// mapping of ROC halves to ECON-D eRx channels
  static const std::vector<std::pair<int, int>> roc_to_erx_map_;
};

class EcalMotherboard {
 public:
  EcalMotherboard() {}
  /// create a module
  void createModule(int imodule, lpGBT& lpGBT, int i2cbus);
  /// get a module
  EcalModule& module(int imodule);

 private:
  std::vector<std::shared_ptr<EcalModule>> modules_;
};

}  // namespace pflib
