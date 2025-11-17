#include "pflib/bittware/bittware_FastControl.h"
#include <unistd.h>

namespace pflib {
namespace bittware {

static constexpr int REG_PULSE                 = 0x100;
static constexpr int REG_CTL                   = 0x600;
static constexpr uint32_t MASK_ENABLE_L1A      = 0x00000001;
static constexpr uint32_t MASK_DISABLE_EXTERNAL= 0x00000002;
static constexpr uint32_t MASK_USE_NZS         = 0x00000004;
static constexpr uint32_t MASK_L1A_PER_ROR     = 0x00001F00;
static constexpr uint32_t MASK_LINK_RESET_BX   = 0xFFF00000;
static constexpr int REG_CALIB_INT             = 0x604;
static constexpr int REG_CALIB_EXT             = 0x608;
static constexpr uint32_t MASK_CALIB_BX        = 0x00000FFF;
static constexpr uint32_t MASK_CALIB_DELTA     = 0x000FF000;

static constexpr int BIT_FIRE_BCR              =  0;
static constexpr int BIT_FIRE_L1A              =  1;
static constexpr int BIT_FIRE_L1ACHAIN         =  2;
static constexpr int BIT_FIRE_OCR              =  4;
static constexpr int BIT_FIRE_EBR              =  5;
static constexpr int BIT_FIRE_ECR              =  6;
static constexpr int BIT_FIRE_LINKRESET_ROCD   =  7;
static constexpr int BIT_FIRE_LINKRESET_ROCT   =  8;
static constexpr int BIT_FIRE_LINKRESET_ECOND  =  9;
static constexpr int BIT_FIRE_LINKRESET_ECONT  = 10;
static constexpr int BIT_FIRE_CALIB_INT        = 11;
static constexpr int BIT_FIRE_CALIB_EXT        = 12;
static constexpr int BIT_RESET_COUNTERS        = 30;

static constexpr int REG_COUNTER_BASE          = 0xC10;

BWFastControl::BWFastControl() : axi_(0x1000) {
  static const int BX_FOR_CALIB = 42;
  axi_.writeMasked(REG_CALIB_INT,MASK_CALIB_BX,BX_FOR_CALIB);
  axi_.writeMasked(REG_CALIB_EXT,MASK_CALIB_BX,BX_FOR_CALIB);
  static const int BX_FOR_LINK_RESET = 40*64-64;
  axi_.writeMasked(REG_CTL,MASK_LINK_RESET_BX,BX_FOR_LINK_RESET);
}

static constexpr const char* names[] = {"BCR","OCR","ECR","EBR","CALIB_INT","CALIB_EXT",
  "LINKRESET_ROCT","LINKRESET_ROCD","LINKRESET_ECONT","LINKRESET_ECOND","L1A","L1A_NZS","INT_ROR","EXT_ROR",0};

std::map<std::string, uint32_t> BWFastControl::getCmdCounters() {
  std::map<std::string, uint32_t> retval;
  for (int i=0; names[i]!=0; i++) {
    retval[names[i]]=axi_.read(REG_COUNTER_BASE+4*i);
  }
  return retval;
}
void BWFastControl::resetCounters() {
  axi_.write(REG_PULSE, 1<<BIT_RESET_COUNTERS);
}
void BWFastControl::sendL1A() {
  axi_.write(REG_PULSE, 1<<BIT_FIRE_L1A);
}
void BWFastControl::sendROR() {
  axi_.write(REG_PULSE, 1<<BIT_FIRE_L1ACHAIN);
}
void BWFastControl::setL1AperROR(int n) {
  l1a_per_ror_=n&0x1F;
  return axi_.writeMasked(REG_CTL,MASK_L1A_PER_ROR,l1a_per_ror_);  
}
int BWFastControl::getL1AperROR() {
  return axi_.readMasked(REG_CTL,MASK_L1A_PER_ROR);
}
void BWFastControl::linkreset_rocs() {
  axi_.write(REG_PULSE, 1<<BIT_FIRE_LINKRESET_ROCD);
  usleep(1000);
  axi_.write(REG_PULSE, 1<<BIT_FIRE_LINKRESET_ROCT);
}
void BWFastControl::linkreset_econs() {
  axi_.write(REG_PULSE, 1<<BIT_FIRE_LINKRESET_ECOND);
  usleep(1000);
  axi_.write(REG_PULSE, 1<<BIT_FIRE_LINKRESET_ECONT);
}
void BWFastControl::bufferclear() {
  axi_.write(REG_PULSE, 1<<BIT_FIRE_EBR);
}
void BWFastControl::orbit_count_reset() {
  axi_.write(REG_PULSE, 1<<BIT_FIRE_OCR);
}
void BWFastControl::chargepulse() {
  axi_.write(REG_PULSE, 1<<BIT_FIRE_CALIB_INT);
}
void BWFastControl::ledpulse() {
  axi_.write(REG_PULSE, 1<<BIT_FIRE_CALIB_EXT);
}
void BWFastControl::clear_run() {
  bufferclear();
  resetCounters();    
}
void BWFastControl::fc_setup_calib(int charge_to_l1a) {
  axi_.writeMasked(REG_CALIB_INT,MASK_CALIB_DELTA,charge_to_l1a);
}
int BWFastControl::fc_get_setup_calib() {
  return axi_.readMasked(REG_CALIB_INT,MASK_CALIB_DELTA);
}
void BWFastControl::fc_setup_led(int charge_to_l1a) {
  axi_.writeMasked(REG_CALIB_EXT,MASK_CALIB_DELTA,charge_to_l1a);
}
int BWFastControl::fc_get_setup_led() {
  return axi_.readMasked(REG_CALIB_EXT,MASK_CALIB_DELTA);
}

void BWFastControl::fc_enables_read(bool& ext_l1a, bool& ext_spill,
                                    bool& timer_l1a) {
  ext_spill=false;
  timer_l1a=false;
  ext_l1a=(axi_.readMasked(REG_CTL, MASK_DISABLE_EXTERNAL)==0);
}  
void BWFastControl::fc_enables(bool ext_l1a, bool ext_spill, bool timer_l1a) {
  if (ext_l1a) axi_.writeMasked(REG_CTL, MASK_DISABLE_EXTERNAL,0);
  else axi_.writeMasked(REG_CTL, MASK_DISABLE_EXTERNAL,1);
}


}
}

