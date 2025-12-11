#pragma once

#include <map>
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
  EcalModule(lpGBT& lpgbt, int i2cbus, int modulenumber);

  /** number of hgcrocs */
  int nrocs() const { return 6; }
  /// number of econds
  int necons() const { return 2; }

  /** do we have a roc with this id? */
  bool have_roc(int iroc) const { return iroc >= 0 && iroc <= nrocs(); }

  static constexpr const int ECON_D = 0;
  static constexpr const int ECON_T = 1;

  /** do we have an econ with this id? */
  bool have_econ(int iecon) const { return iecon == ECON_D || iecon == ECON_T; }

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
  std::unique_ptr<I2C> i2c_;
  /// representation of Ecal HexaModule
  std::vector<ROC> rocs_;
  std::vector<ECON> econs_;
  // Mapping ROC channel → ERX channel
 private:
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
  std::vector<std::shared_ptr<EcalModule*>> modules_;
};

}  // namespace pflib
