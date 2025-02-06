#include "pflib/Target.h"
#include "pflib/I2C_Linux.h"
#include "pflib/GPIO.h"
#include <iostream>
#include <unistd.h>

namespace pflib {

class HcalFiberless : public Hcal {
 public:
  static const int GPO_HGCROC_RESET_HARD = 2;
  static const int GPO_HGCROC_RESET_SOFT = 1;
  static const int GPO_HGCROC_RESET_I2C  = 0;

  HcalFiberless(const std::vector<std::shared_ptr<I2C>>& roc_i2c) : Hcal(roc_i2c) {
    gpio_.reset(make_GPIO_HcalHGCROCZCU());

    // should already be done, but be SURE
    gpio_->setGPO(GPO_HGCROC_RESET_HARD, true);
    gpio_->setGPO(GPO_HGCROC_RESET_SOFT, true);
    gpio_->setGPO(GPO_HGCROC_RESET_I2C, true);

    elinks_=get_Elinks_zcu();
    daq_=get_DAQ_zcu();
  }
  
  virtual void hardResetROCs() {
    gpio_->setGPO(GPO_HGCROC_RESET_HARD, false); // active low
    gpio_->setGPO(GPO_HGCROC_RESET_I2C, false); // active low
    usleep(10); printf("HARD\n");
    gpio_->setGPO(GPO_HGCROC_RESET_HARD, true); // active low
    gpio_->setGPO(GPO_HGCROC_RESET_I2C, true); // active low
  }

  virtual void softResetROC(int which) {
    gpio_->setGPO(GPO_HGCROC_RESET_SOFT, false); // active low
    gpio_->setGPO(GPO_HGCROC_RESET_SOFT, true); // active low
  }

  virtual Elinks& elinks() { return *elinks_; }
  virtual DAQ& daq() { return *daq_; }
private:
  Elinks* elinks_;
  DAQ* daq_;
};

class TargetFiberless : public Target {
 public:
  TargetFiberless() : Target() {
    
    i2croc_=std::shared_ptr<I2C>(new I2C_Linux("/dev/i2c-24"));
    i2cboard_=std::shared_ptr<I2C>(new I2C_Linux("/dev/i2c-23"));

    i2c_["HGCROC"]=i2croc_;
    i2c_["BOARD"]=i2cboard_;
    i2c_["BIAS"]=i2cboard_;

    std::vector<std::shared_ptr<I2C>> roc_i2cs;
    roc_i2cs.push_back(i2croc_);

    hcal_=std::shared_ptr<Hcal>(new HcalFiberless(roc_i2cs));
    fc_=std::shared_ptr<FastControl>(make_FastControlCMS_MMap());
  }

  virtual std::vector<uint32_t> read_event();

 private:
  std::shared_ptr<I2C> i2croc_;
  std::shared_ptr<I2C> i2cboard_;
 
};

std::vector<uint32_t> TargetFiberless::read_event() {
  std::vector<uint32_t> buffer;
  if (has_event()) {
    buffer.push_back(0x11888811);
    buffer.push_back(0xbeef2025);
    buffer.push_back(0); // come back to this
    for (int i=0; i<(hcal().daq().nlinks()+1)/2; i++)
      buffer.push_back(0); // come back to this
    size_t len_total=buffer.size()-2;

    for (int i=0; i<hcal().daq().nlinks(); i++) {
      std::vector<uint32_t> data=hcal().daq().getLinkData(i);
      if (i>=2) { // trigger links
	uint32_t theader=0x30000000|((i-2))|(data.size()<<8);
	data.insert(data.begin(),theader);
      }
      size_t len=data.size();
      len_total+=len;
      buffer.insert(buffer.end(),data.begin(),data.end());
      // insert the subpacket length
      if (i%2) buffer[2+1+i/2]|=(len<<16);
      else buffer[2+1+i/2]|=(len);
    }
    // record the total length
    buffer[2]|=len_total;
    buffer.push_back(0xd07e2025);
    buffer.push_back(0x12345678);
    hcal().daq().advanceLinkReadPtr();
  }
  return buffer;
}

Target* makeTargetFiberless() { return new TargetFiberless(); }

}
