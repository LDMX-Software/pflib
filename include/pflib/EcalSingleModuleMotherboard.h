#pragma once
#ifndef PFLIB_ECALSINGLEMODULEMOTHERBOARD_H
#define PFLIB_ECALSINGLEMODULEMOTHERBOARD_H

#include "pflib/EcalModule.h"
#include "pflib/Target.h"

namespace pflib {

/**
 * a target for Ecal motherboards holding a single module
 *
 * common accessors and initialization procedures between
 * the ZCU and Bittware targets
 * Since there is just one module, the ROC/ECON
 * accessors and resets are just forwarding calls to our
 * held instance of an EcalModule.
 */
class EcalSingleModuleMotherboard : public Target {
 public:
  virtual ~EcalSingleModuleMotherboard() = default;
  /**
   * common initialization given the DAQ/TRG lpGBTs and
   * a mask labeling which ROCs on the module are active
   *
   * This should be called after the optical links are
   * properly setup and used to create access to the lpGBTs.
   */
  void init(lpGBT& daq_lpgbt, lpGBT& trg_lpgbt, int module_i2c_bus, int roc_mask);
  virtual const std::vector<std::pair<int,int>>& getRocErxMapping() override;
  virtual int nrocs() override;
  virtual int necons() override;
  virtual bool have_roc(int iroc) const override;
  virtual bool have_econ(int iecon) const override;
  virtual std::vector<int> roc_ids() const override;
  virtual std::vector<int> econ_ids() const override;
  virtual ROC& roc(int which) override;
  virtual ECON& econ(int which) override;
  virtual void softResetROC(int which) override;
  virtual void softResetECON(int which = -1) override;
  virtual void hardResetROCs() override;
  virtual void hardResetECONs() override;
  virtual void setup_run(int irun, Target::DaqFormat format, int contrib_id) override;
 protected:
  /// handle to the hexa-module that we are connected to
  std::shared_ptr<EcalModule> the_module_;
  /// the format we are using for DAQ
  Target::DaqFormat format_;
  /// the contributor ID for DAQ
  int contrib_id_;
};

}

#endif
