#include <unistd.h>

#include <iostream>
#include <memory>

#include "pflib/ECOND_Formatter.h"
#include "pflib/Elinks.h"
#include "pflib/HcalBackplane.h"
#include "pflib/I2C_Linux.h"

namespace pflib {

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

    rocs_.emplace(std::piecewise_construct,
        std::forward_as_tuple(0),
        std::forward_as_tuple(i2croc, 0x20, "sipm_rocv3b")
    );
    biases_.emplace(std::piecewise_construct,
        std::forward_as_tuple(0),
        std::forward_as_tuple(i2cboard, i2cboard)
    );

    gpio_.reset(make_GPIO_HcalHGCROCZCU());

    // should already be done, but be SURE
    gpio_->setGPO(GPO_HGCROC_RESET_HARD, true);
    gpio_->setGPO(GPO_HGCROC_RESET_SOFT, true);
    gpio_->setGPO(GPO_HGCROC_RESET_I2C, true);

    elinks_ = std::shared_ptr<Elinks>(get_Elinks_zcu());
    daq_ = std::shared_ptr<DAQ>(get_DAQ_zcu());

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

  virtual Elinks& elinks() override { return *elinks_; }
  virtual DAQ& daq() override { return *daq_; }
  virtual FastControl& fc() override { return *fc_; }
  virtual void setup_run(int run, Target::DaqFormat format, int contrib_id);
  virtual std::vector<uint32_t> read_event();

 public:
  std::shared_ptr<FastControl> fc_;
  std::shared_ptr<Elinks> elinks_;
  std::shared_ptr<DAQ> daq_;
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
