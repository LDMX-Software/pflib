#include <unistd.h>

#include <iostream>
#include <memory>

#include "pflib/Bias.h"
#include "pflib/Exception.h"
#include "pflib/GPIO.h"
#include "pflib/I2C_Linux.h"
#include "pflib/Target.h"
#include "pflib/packing/DAQSampleHeader.h"
#include "pflib/packing/ECONDFormatter.h"
#include "pflib/zcu/UIO.h"

namespace pflib {

/*
 * Without intermediate boards, we can join the Elinks and DAQ
 * roles into one structure that the target can hold.
 */
class FiberlessCapture : public Elinks, public DAQ {
 public:
  FiberlessCapture()
      : Elinks(6),
        DAQ(6),
        uio_("ldmx_buffer", 16 * 4096) {  // currently covers all elinks
  }

  virtual int getBitslip(int ilink);
  virtual uint32_t getStatusRaw(int ilink) { return 0; }
  virtual void setBitslip(int ilink, int bitslip);
  virtual std::vector<uint32_t> spy(int ilink);
  virtual void clearErrorCounters(int ilink) {}
  virtual void resetHard() {}
  virtual void setAlignPhase(int ilink, int phase);
  virtual int getAlignPhase(int ilink);

  // DAQ-related
  virtual void reset();
  virtual int getEventOccupancy();
  virtual void setupLink(int ilink, int l1a_delay, int l1a_capture_width);
  virtual void getLinkSetup(int ilink, int& l1a_delay, int& l1a_capture_width);
  virtual void bufferStatus(int ilink, bool& empty, bool& full);
  virtual std::vector<uint32_t> getLinkData(int ilink);
  virtual void advanceLinkReadPtr();

 private:
  int ctl_for(int ilink) {
    if (ilink < 2)
      return 4 + ilink;
    else
      return 4 + 4 + (ilink - 2);
  }
  UIO uio_;
  std::vector<int> l1a_capture_width_;
};

static const uint32_t MASK_CAPTURE_WIDTH = 0x3F000000;
static const uint32_t MASK_CAPTURE_DELAY = 0x00FF0000;
static const uint32_t MASK_BITSLIP = 0x00003E00;
static const uint32_t MASK_PHASE = 0x000001FF;

static const uint32_t MASK_RESET_BUFFER = 0x00010000;
static const uint32_t MASK_ADVANCE_FIFO = 0x00020000;
static const uint32_t MASK_SOFTWARE_L1A = 0x00040000;

static const uint32_t MASK_OCCUPANCY = 0x000000FF;
static const uint32_t MASK_BUFFER_FULL = 0x00000100;
static const uint32_t MASK_BUFFER_EMPTY = 0x00000200;

static const size_t ADDR_TOP_CTL = 0x0;
static const size_t ADDR_LINK_STATUS_BASE = 0x26;
static const size_t ADDR_OFFSET_BUFSTATUS = 1;

class HcalFiberless : public Target {
 public:
  static constexpr const char* GPO_HGCROC_RESET_HARD = "HGCROC_HARD_RSTB";
  static constexpr const char* GPO_HGCROC_RESET_SOFT = "HGCROC_SOFT_RSTB";
  static constexpr const char* GPO_HGCROC_RESET_I2C = "HGCROC_RSTB_I2C";

  const std::vector<std::pair<int, int>>& getHardwareRocErxMapping() override {
    static const std::vector<std::pair<int, int>> THE_MAP = {{0, 1}};
    return THE_MAP;
  }
  virtual Bias bias(int which) {
    if (which == 0) return *bias_;
    PFEXCEPTION_RAISE("NoMore",
                      "Only one bias board (index=0) for fiberless setup.");
  }
  virtual ROC& roc(int which) override {
    if (which == 0) return *roc_;
    PFEXCEPTION_RAISE("NoMore", "Only one ROC (index=0) for fiberless setup.");
  }
  virtual ECON& econ(int which) override {
    PFEXCEPTION_RAISE("InvalidECONid",
                      "No ECONs connected for Fiberless targets.");
  }
  virtual void hardResetECONs() override {
    PFEXCEPTION_RAISE("Invalid", "No ECONs connected for Fiberless targets.");
  }
  virtual void softResetECON(int which = -1) override {
    PFEXCEPTION_RAISE("Invalid", "No ECONs connected for Fiberless targets.");
  }
  virtual GPIO& gpio() { return *gpio_; }
  virtual int nrocs() override { return 1; }
  virtual bool have_roc(int i) const override { return (i == 0); }
  virtual std::vector<int> roc_ids() const override { return {0}; }
  virtual int necons() override { return 0; }
  virtual bool have_econ(int iecon) const override { return false; }
  virtual std::vector<int> econ_ids() const override { return {}; }

  HcalFiberless() : Target() {
    auto i2croc = std::shared_ptr<I2C>(new I2C_Linux("/dev/i2c-24"));
    if (not i2croc) {
      PFEXCEPTION_RAISE("I2CError", "Could not open ROC I2C bus");
    }
    auto i2cboard = std::shared_ptr<I2C>(new I2C_Linux("/dev/i2c-23"));
    if (not i2cboard) {
      PFEXCEPTION_RAISE("I2CError", "Could not open bias I2C bus");
    }

    roc_ = std::make_unique<ROC>(i2croc, 0x20, "sipm_rocv3b");
    bias_ = std::make_unique<Bias>(i2cboard, i2cboard);

    gpio_.reset(make_GPIO_HcalHGCROCZCU());

    // should already be done, but be SURE
    gpio_->setGPO(GPO_HGCROC_RESET_HARD, true);
    gpio_->setGPO(GPO_HGCROC_RESET_SOFT, true);
    gpio_->setGPO(GPO_HGCROC_RESET_I2C, true);

    capture_ = std::make_shared<FiberlessCapture>();

    i2c_["HGCROC"] = i2croc;
    i2c_["BOARD"] = i2cboard;
    i2c_["BIAS"] = i2cboard;

    fc_ = std::shared_ptr<FastControl>(make_FastControlCMS_MMap());
  }

  virtual void hardResetROCs() override {
    gpio_->setGPO(GPO_HGCROC_RESET_HARD, false);  // active low
    gpio_->setGPO(GPO_HGCROC_RESET_I2C, false);   // active low
    usleep(10);
    gpio_->setGPO(GPO_HGCROC_RESET_HARD, true);  // active low
    gpio_->setGPO(GPO_HGCROC_RESET_I2C, true);   // active low
  }

  virtual void softResetROC(int which) override {
    gpio_->setGPO(GPO_HGCROC_RESET_SOFT, false);  // active low
    gpio_->setGPO(GPO_HGCROC_RESET_SOFT, true);   // active low
  }

  virtual Elinks& elinks() override { return *capture_; }
  virtual DAQ& daq() override { return *capture_; }
  virtual FastControl& fc() override { return *fc_; }
  virtual void setup_run(int run, Target::DaqFormat format, int contrib_id);
  virtual std::vector<uint32_t> read_event();

 public:
  std::unique_ptr<GPIO> gpio_;
  std::shared_ptr<FastControl> fc_;
  std::shared_ptr<FiberlessCapture> capture_;
  std::unique_ptr<ROC> roc_;
  std::unique_ptr<Bias> bias_;
  int run_;
  Target::DaqFormat daqformat_;
  int ievt_, l1a_;
  int contribid_;
  packing::ECONDFormatter formatter_{true};
};

static const int SUBSYSTEM_ID_HCAL_DAQ = 0x07;

}
