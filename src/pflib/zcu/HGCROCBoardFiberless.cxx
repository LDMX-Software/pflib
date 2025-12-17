#include <unistd.h>

#include <iostream>
#include <memory>

#include "pflib/zcu/UIO.h"
#include "pflib/ECOND_Formatter.h"
#include "pflib/HcalBackplane.h"
#include "pflib/I2C_Linux.h"

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

int FiberlessCapture::getBitslip(int ilink) {
  int ictl = ctl_for(ilink);
  return uio_.readMasked(ictl, MASK_BITSLIP);
}

void FiberlessCapture::setBitslip(int ilink, int bitslip) {
  int ictl = ctl_for(ilink);
  uio_.writeMasked(ictl, MASK_BITSLIP, bitslip);
}

void FiberlessCapture::setAlignPhase(int ilink, int phase) {
  int ictl = ctl_for(ilink);
  uio_.writeMasked(ictl, MASK_PHASE, phase);
}

int FiberlessCapture::getAlignPhase(int ilink) {
  int ictl = ctl_for(ilink);
  return uio_.readMasked(ictl, MASK_PHASE);
}

std::vector<uint32_t> FiberlessCapture::spy(int ilink) {
  std::vector<uint32_t> rv;
  int spy_addr = ADDR_LINK_STATUS_BASE + 2 * ilink;
  rv.push_back(uio_.read(spy_addr));
  return rv;
  ;
}

void FiberlessCapture::reset() {
  uio_.rmw(ADDR_TOP_CTL, MASK_RESET_BUFFER, MASK_RESET_BUFFER);
}
int FiberlessCapture::getEventOccupancy() {
  // use link 0 for occupancy
  return uio_.readMasked(ADDR_LINK_STATUS_BASE + ADDR_OFFSET_BUFSTATUS,
                         MASK_OCCUPANCY);
}
void FiberlessCapture::setupLink(int ilink, int l1a_delay, int l1a_capture_width) {
  int ictl = ctl_for(ilink);
  // gather lengths if we don't have them
  if (l1a_capture_width_.empty()) {
    int a, b;
    for (int i = 0; i < DAQ::nlinks(); i++) {
      getLinkSetup(i, a, b);
      l1a_capture_width_.push_back(b);
    }
  }
  l1a_capture_width_[ilink] = l1a_capture_width;

  uio_.writeMasked(ictl, MASK_CAPTURE_DELAY, l1a_delay);
  uio_.writeMasked(ictl, MASK_CAPTURE_WIDTH, l1a_capture_width);
}
void FiberlessCapture::getLinkSetup(int ilink, int& l1a_delay,
                               int& l1a_capture_width) {
  int ictl = ctl_for(ilink);
  l1a_delay = uio_.readMasked(ictl, MASK_CAPTURE_DELAY);
  l1a_capture_width = uio_.readMasked(ictl, MASK_CAPTURE_WIDTH);
}
void FiberlessCapture::bufferStatus(int ilink, bool& empty, bool& full) {
  empty = true;
  full = true;  // implauible
  if (ilink < 0 || ilink >= DAQ::nlinks()) return;
  uint32_t ifl =
      uio_.readMasked(ADDR_LINK_STATUS_BASE + ADDR_OFFSET_BUFSTATUS + ilink * 2,
                      MASK_BUFFER_FULL);
  uint32_t iemp =
      uio_.readMasked(ADDR_LINK_STATUS_BASE + ADDR_OFFSET_BUFSTATUS + ilink * 2,
                      MASK_BUFFER_EMPTY);
  empty = (iemp != 0);
  full = (ifl != 0);
}

std::vector<uint32_t> FiberlessCapture::getLinkData(int ilink) {
  std::vector<uint32_t> rv;
  if (ilink < 0 || ilink >= DAQ::nlinks()) return rv;
  // gather lengths if we don't have them
  if (l1a_capture_width_.empty()) {
    int a, b;
    for (int i = 0; i < DAQ::nlinks(); i++) {
      getLinkSetup(i, a, b);
      l1a_capture_width_.push_back(b);
    }
  }

  size_t addr = 0x200 | (ilink << 6);
  // printf("%d %d %x\n", ilink, addr, addr);
  for (size_t i = 0; i < l1a_capture_width_[ilink]; i++)
    rv.push_back(uio_.read(addr + i));
  return rv;
}

void FiberlessCapture::advanceLinkReadPtr() {
  if (getEventOccupancy() > 0)
    uio_.rmw(ADDR_TOP_CTL, MASK_ADVANCE_FIFO, MASK_ADVANCE_FIFO);
}

class HcalFiberless : public HcalBackplane {
 public:
  static constexpr const char* GPO_HGCROC_RESET_HARD = "HGCROC_HARD_RSTB";
  static constexpr const char* GPO_HGCROC_RESET_SOFT = "HGCROC_SOFT_RSTB";
  static constexpr const char* GPO_HGCROC_RESET_I2C = "HGCROC_RSTB_I2C";

  virtual int nrocs() override { return 1; }
  virtual int necons() override { return 0; }
  virtual bool have_roc(int i) const override { return (i==0); }
  virtual std::vector<int> roc_ids() const override { return {0}; }

  HcalFiberless() : HcalBackplane() {
    auto i2croc = std::shared_ptr<I2C>(new I2C_Linux("/dev/i2c-24"));
    if (not i2croc) {
      PFEXCEPTION_RAISE("I2CError", "Could not open ROC I2C bus");
    }
    auto i2cboard = std::shared_ptr<I2C>(new I2C_Linux("/dev/i2c-23"));
    if (not i2cboard) {
      PFEXCEPTION_RAISE("I2CError", "Could not open bias I2C bus");
    }

    rocs_[0] = std::make_unique<HGCROCBoard>(
        ROC(i2croc, 0x20, "sipm_rocv3b"),
        Bias(i2cboard, i2cboard)
    );
    nhgcroc_++;

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

  ECON& econ(int which) override {
    PFEXCEPTION_RAISE("InvalidECONid",
        "No ECONs connected for Fiberless targets.");
  }

  virtual Elinks& elinks() override { return *capture_; }
  virtual DAQ& daq() override { return *capture_; }
  virtual FastControl& fc() override { return *fc_; }
  virtual void setup_run(int run, Target::DaqFormat format, int contrib_id);
  virtual std::vector<uint32_t> read_event();

 public:
  std::shared_ptr<FastControl> fc_;
  std::shared_ptr<FiberlessCapture> capture_;
  int run_;
  Target::DaqFormat daqformat_;
  int ievt_, l1a_;
  int contribid_;
  ECOND_Formatter formatter_;
};

static const int SUBSYSTEM_ID_HCAL_DAQ = 0x07;

void HcalFiberless::setup_run(int run, DaqFormat format, int contrib_id) {
  run_ = run;
  daqformat_ = format;
  if (contrib_id < 0)
    contribid_ = 255;
  else
    contribid_ = contrib_id & 0xFF;
  ievt_ = 0;
  l1a_ = 0;
  daq().reset();
  fc().clear_run();
}

std::vector<uint32_t> HcalFiberless::read_event() {
  std::vector<uint32_t> buffer;
  if (has_event()) {
    ievt_++;
    switch (daqformat_) {
      case DaqFormat::SIMPLEROC: {
        buffer.push_back(0x11888811);
        buffer.push_back(0xbeef2025);

        buffer.push_back(0);  // come back to this
        for (int i = 0; i < (daq().nlinks() + 1) / 2; i++)
          buffer.push_back(0);  // come back to this
        size_t len_total = buffer.size() - 2;

        for (int i = 0; i < daq().nlinks(); i++) {
          std::vector<uint32_t> data = daq().getLinkData(i);
          if (i >= 2) {  // trigger links
            uint32_t theader = 0x30000000 | ((i - 2)) | (data.size() << 8);
            data.insert(data.begin(), theader);
          }
          size_t len = data.size();
          len_total += len;
          buffer.insert(buffer.end(), data.begin(), data.end());
          // insert the subpacket length
          if (i % 2)
            buffer[2 + 1 + i / 2] |= (len << 16);
          else
            buffer[2 + 1 + i / 2] |= (len);
        }
        // record the total length
        buffer[2] |= len_total;
        buffer.push_back(0xd07e2025);
        buffer.push_back(0x12345678);
        daq().advanceLinkReadPtr();
      } break;
      case DaqFormat::ECOND_NO_ZS: {
        const int bc = 0;  // bx number...
        /*
        buffer.push_back(0xb33f2025);
        buffer.push_back(run_);
        buffer.push_back((ievt_ << 8) | bc);
        buffer.push_back(0);
        buffer.push_back((0xA6u << 24) | (contribid_ << 16) |
                         (SUBSYSTEM_ID_HCAL_DAQ << 8) | (0));
        */

        for (int il1a = 0; il1a < daq().samples_per_ror(); il1a++) {
          // assume orbit zero, L1A spaced by two
          formatter_.startEvent(bc + il1a * 2, l1a_ + il1a, 0);
          // only consuming DAQ links in ECOND (D for DAQ)
          for (int i = 0; i < 2; i++) {
            formatter_.add_elink_packet(i, daq().getLinkData(i));
          }
          formatter_.finishEvent();

          // add header giving specs around ECOND packet
          uint32_t header = formatter_.getPacket().size();
          header |= (0x1 << 28);
          header |= (daq().econid() & 0x3ff) << 18;
          header |= (il1a & 0x1f) << 13;
          if (il1a == daq().soi()) header |= (1 << 12);
          buffer.push_back(header);

          // insert ECOND packet into buffer
          buffer.insert(buffer.end(), formatter_.getPacket().begin(),
                        formatter_.getPacket().end());

          // advance L1A pointer
          daq().advanceLinkReadPtr();
        }
        l1a_ += daq().samples_per_ror();
        // add a special "header" to mark that we have no more ECON packets
        uint32_t header{0};
        header |= (0x1 << 28);
        header |= (daq().econid() & 0x3ff) << 18;
        buffer.push_back(header);
        /*
        buffer.push_back(0x12345678);
        */
      } break;
      default: {
        PFEXCEPTION_RAISE("NoImpl", "DaqFormat provided is not implemented");
      }
    }
  }
  return buffer;
}

Target* makeTargetFiberless() { return new HcalFiberless(); }

}  // namespace pflib
