#include "pflib/Elinks.h"

namespace pflib {

static const int REG_CONTROL               = 1;
static const int CTL_SPYSTART              = 0x10000000;
static const int CTL_SPYPICK_MASK          = 0x00001F00;
static const int CTL_SPYPICK_SHIFT         = 8;

static const int LINK_CTL_BASE             = 0x40;
static const int LINK_CTL_BITSLIP_MASK     = 0x70;
static const int LINK_CTL_BITSLIP_SHIFT    = 4;
static const int LINK_STATUS_BASE          = 0x80;

static const int SPY_WINDOW           = 0xC0;

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
}

int Elinks::getBitslip(int ilink) {
  return (wb_read(LINK_CTL_BASE+ilink)|LINK_CTL_BITSLIP_MASK)>>LINK_CTL_BITSLIP_SHIFT;
}

uint32_t Elinks::getStatusRaw(int ilink) {
  return wb_read(LINK_STATUS_BASE+ilink);
}


}
