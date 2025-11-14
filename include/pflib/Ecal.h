#pragma once

#include "pflib/ECON.h"
#include "pflib/ROC.h"
#include "pflib/Target.h"

namespace pflib {

/**
 * Abstract class holding a set of ROCs and ECONs representing a HexaModule
 */
class Ecal {
 public:
  /** number of hgcrocs */
  virtual int nrocs() override { return modules_.rocs_.size(); }

  /// number of econds
  virtual int necons() override { return modules_.econs_.size(); }

  /** do we have a roc with this id? */
  virtual bool have_roc(int iroc) const override;

  /** do we have an econ with this id? */
  virtual bool have_econ(int iecon) const override;

  /** get a list of the IDs we have set up */
  virtual std::vector<int> roc_ids() const override;

  /** get a list of the econ IDs we have set up */
  virtual std::vector<int> econ_ids() const override;

  /** Get a ROC interface for the given HGCROC board */
  virtual ROC roc(int which) override;

  /** get a ECON interface for the given econ board */
  virtual ECON econ(int which) override;

  /** Generate a hard reset to all the HGCROC boards */
  virtual void hardResetROCs() override;

  /** generate a hard reset to all the ECON boards */
  virtual void hardResetECONs() override;

  /** Get the firmware version */
  virtual uint32_t getFirmwareVersion() override;

  /** Generate a soft reset to a specific HGCROC board, -1 for all */
  virtual void softResetROC(int which = -1) override;

  /** Generate a soft reset to a specific ECON board, -1 for all */
  virtual void softResetECON(int which = -1) override;

 protected:
  /// representation of Ecal HexaModule
  struct Module {
    std::vector<ROC> rocs_;
    std::vector<ECON> econs_;
  };
  std::vector<Module> modules_;

  /// an Ecal Motherboard
  struct Motherboard {
    std::vector<lpGBT> lpgbts_;
  };
  std::vector<Motherboard> motherboards_;
};

}  // namespace pflib
