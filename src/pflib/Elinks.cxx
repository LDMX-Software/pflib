#include "pflib/Elinks.h"
#include <unistd.h>

namespace pflib {

static const int REG_CONTROL               = 1;
static const int CTL_SPYSTART              = 0x10000000;
static const int CTL_SPYPICK_MASK          = 0x00001F00;
static const int CTL_SPYPICK_SHIFT         = 8;
static const uint32_t CTL_RESET_HARD       = 0x20000000;
static const uint32_t CTL_AUTOALIGN_THRESHOLD_MASK = 0xF00;
static const int CTL_AUTOALIGN_THRESHOLD_SHIFT = 8;

static const int LINK_CTL_BASE             = 0x40;
static const int LINK_CTL_BITSLIP_MASK     = 0x70;
static const int LINK_CTL_BITSLIP_SHIFT    = 4;
static const int LINK_CTL_AUTO_ENABLE      = 0x80;
static const int LINK_CTL_DELAY_DIR        = 0x00000001;
static const int LINK_CTL_EYE_CLEAR        = 0x00010000;
static const int LINK_CTL_DELAY_MOVE       = 0x00040000;
static const int LINK_CTL_DELAY_LOAD       = 0x00080000;
static const int LINK_CTL_CLEAR_COUNTERS   = 0x00100000;
static const int LINK_CTL_RESET_AUTO       = 0x00200000;

static const int LINK_STATUS_BASE          = 0x80;

static const int LINK_STATUS_DELAYRANGE    = 0x01;
static const int LINK_STATUS_EYE_EARLY     = 0x02;
static const int LINK_STATUS_EYE_LATE      = 0x04;
static const int LINK_STATUS_ALIGNED       = 0x10;
static const int LINK_STATUS_AUTOLOCK      = 0x20;
static const int LINK_STATUS_AUTOPHASE_MASK =  0x700;
static const int LINK_STATUS_AUTOPHASE_SHIFT =  8;
static const int LINK_STATUS_BADIDLE_MASK =  0xFFF0000;
static const int LINK_STATUS_BADIDLE_SHIFT =  16;
static const int LINK_STATUS_AUTOCYCLE_MASK =  0xF0000000u;
static const int LINK_STATUS_AUTOCYCLE_SHIFT =  28;

static const int BIGSPY_REG                = 2;
static const int BIGSPY_LINK_MASK          = 0x000000F0;
static const int BIGSPY_LINK_SHIFT         = 4;
static const int BIGSPY_MODE_MASK          = 0x0000000F;
static const int BIGSPY_MODE_SHIFT         = 0;
static const int BIGSPY_NAFTER_MASK        = 0x03FF0000;
static const int BIGSPY_NAFTER_SHIFT       = 16;
static const int BIGSPY_RESET              = 0x80000000u;
static const int BIGSPY_SW_TRIGGER         = 0x40000000u;
static const int BIGSPY_DONE               = 0x20000000u;

static const int SPY_WINDOW                = 0xC0;
static const int BIGSPY_WINDOW             = 0x400;

std::vector<uint8_t> Elinks::spy(int ilink) {
  wb_rmw(REG_CONTROL,CTL_SPYSTART,CTL_SPYSTART); // do the spy now...
  wb_rmw(REG_CONTROL,ilink<<CTL_SPYPICK_SHIFT,CTL_SPYPICK_MASK); // pick a winner
  std::vector<uint8_t> retval;
  for (int i=0; i<64; i++)
    retval.push_back(uint8_t(wb_read(SPY_WINDOW+i)));
  return retval;
}

void Elinks::setBitslip(int ilink, int bitslip) {
  wb_rmw(LINK_CTL_BASE+ilink,bitslip<<LINK_CTL_BITSLIP_SHIFT,LINK_CTL_BITSLIP_MASK);
  wb_rmw(LINK_CTL_BASE+ilink,0,LINK_CTL_AUTO_ENABLE);
}

void Elinks::setBitslipAuto(int ilink) {
  wb_rmw(LINK_CTL_BASE+ilink,LINK_CTL_AUTO_ENABLE,LINK_CTL_AUTO_ENABLE);
}

bool Elinks::isBitslipAuto(int ilink) {
  return wb_read(LINK_CTL_BASE+ilink)&LINK_CTL_AUTO_ENABLE;
}

int Elinks::getBitslip(int ilink) {
  if (isBitslipAuto(ilink)) return (wb_read(LINK_STATUS_BASE+ilink)&LINK_STATUS_AUTOPHASE_MASK)>>LINK_STATUS_AUTOPHASE_SHIFT;
  else return (wb_read(LINK_CTL_BASE+ilink)&LINK_CTL_BITSLIP_MASK)>>LINK_CTL_BITSLIP_SHIFT;
}

uint32_t Elinks::getStatusRaw(int ilink) {
  return wb_read(LINK_STATUS_BASE+ilink);
}

void Elinks::clearErrorCounters(int ilink) {
  wb_rmw(LINK_CTL_BASE+ilink,LINK_CTL_CLEAR_COUNTERS,LINK_CTL_CLEAR_COUNTERS);
}
void Elinks::readCounters(int ilink, uint32_t& nonidles, uint32_t& resyncs) {
  uint32_t val=getStatusRaw(ilink);
  nonidles=(val&LINK_STATUS_BADIDLE_MASK)>>LINK_STATUS_BADIDLE_SHIFT;
  resyncs=(val& LINK_STATUS_AUTOCYCLE_MASK)>> LINK_STATUS_AUTOCYCLE_SHIFT;
}

void Elinks::resetHard() {
  wb_rmw(REG_CONTROL,CTL_RESET_HARD,CTL_RESET_HARD);
}

void Elinks::setupBigspy(int mode, int ilink, int presamples) {
  uint32_t val;
  val=mode&BIGSPY_MODE_MASK;
  val|=(ilink<<BIGSPY_LINK_SHIFT)&BIGSPY_LINK_MASK;
  uint32_t nafter=0x3ff-presamples;
  val|=(nafter<<BIGSPY_NAFTER_SHIFT)&BIGSPY_NAFTER_MASK;
  val|=BIGSPY_RESET;
  wb_write(BIGSPY_REG,val);
}

void Elinks::getBigspySetup(int& mode, int& ilink, int& presamples) {
  uint32_t val=wb_read(BIGSPY_REG);
  mode=(val&BIGSPY_MODE_MASK)>>BIGSPY_MODE_SHIFT;
  ilink=(val&BIGSPY_LINK_MASK)>>BIGSPY_LINK_SHIFT;
  uint32_t nafter=(val&BIGSPY_NAFTER_MASK)>>BIGSPY_NAFTER_SHIFT;
  presamples=0x3ff-nafter;
}

bool Elinks::bigspyDone() {
  uint32_t val=wb_read(BIGSPY_REG);
  return (val&BIGSPY_DONE)!=0;
}

std::vector<uint32_t> Elinks::bigspy() {
  uint32_t val=wb_read(BIGSPY_REG);
  // if software trigger, need to fire it...
  if ((val&BIGSPY_MODE_MASK)==0 && !(val&BIGSPY_DONE)) {
    wb_rmw(BIGSPY_REG, BIGSPY_SW_TRIGGER, BIGSPY_SW_TRIGGER);
    usleep(10);
  }
  std::vector<uint32_t> rv;
  for (int i=0; i<0x400; i++)
    rv.push_back(wb_read(BIGSPY_WINDOW+i));
  return rv;
}

void Elinks::scanAlign(int ilink) {
  uint32_t status;
  // first, reload to the beginning
  wb_rmw(LINK_CTL_BASE+ilink,LINK_CTL_DELAY_LOAD,LINK_CTL_DELAY_LOAD);
  // now, step until we get range edge
  wb_rmw(LINK_CTL_BASE+ilink,0,LINK_CTL_DELAY_DIR|LINK_CTL_DELAY_MOVE);
  do {
    wb_rmw(LINK_CTL_BASE+ilink,LINK_CTL_DELAY_MOVE,LINK_CTL_DELAY_MOVE);
    status=getStatusRaw(ilink);
  } while ((status&LINK_STATUS_DELAYRANGE)==0);
  // change direction and start stepping...
  int istep=0;
  wb_rmw(LINK_CTL_BASE+ilink,1,LINK_CTL_DELAY_DIR);
  do {
    wb_rmw(LINK_CTL_BASE+ilink,LINK_CTL_DELAY_MOVE,LINK_CTL_DELAY_MOVE);
    wb_rmw(LINK_CTL_BASE+ilink,LINK_CTL_EYE_CLEAR,LINK_CTL_EYE_CLEAR);
    status=getStatusRaw(ilink);
    printf("%d %08x\n",istep,status);
    istep++;
  } while ((status&LINK_STATUS_DELAYRANGE)==0);
}


void Elinks::setDelay(int ilink,int idelay) {

  uint32_t status;
  // first, reload to the beginning
  wb_rmw(LINK_CTL_BASE+ilink,LINK_CTL_DELAY_LOAD,LINK_CTL_DELAY_LOAD);
  // now, step until we get range edge
  wb_rmw(LINK_CTL_BASE+ilink,0,LINK_CTL_DELAY_DIR|LINK_CTL_DELAY_MOVE);
  do {
    wb_rmw(LINK_CTL_BASE+ilink,LINK_CTL_DELAY_MOVE,LINK_CTL_DELAY_MOVE);
    status=getStatusRaw(ilink);
  } while ((status&LINK_STATUS_DELAYRANGE)==0);
  // change direction and start stepping...
  wb_rmw(LINK_CTL_BASE+ilink,1,LINK_CTL_DELAY_DIR);
  for (int istep=0; istep<idelay; istep++) {
    wb_rmw(LINK_CTL_BASE+ilink,LINK_CTL_DELAY_MOVE,LINK_CTL_DELAY_MOVE);
  }
}

}
