#include "pflib/DAQ.h"
#include "pflib/WishboneTarget.h"

namespace pflib {

static const int CTL_OCCUPANCY_REG       = 4;
static const uint32_t CTL_OCCUPANCY_MASK = 0x3F;
static const int CTL_OCCUPANCY_CURRENT_SHIFT = 8;
static const int CTL_OCCUPANCY_MAXIMUM_SHIFT = 0;
static const int CTL_CONTROL_REG        = 1;
static const uint32_t CTL_RESET              = 0x20000000;
static const uint32_t CTL_LINK_COUNT_MASK    = 0xFF00;
static const int CTL_LINK_COUNT_SHIFT   = 8;
static const int CTL_FPGAID_REG         = 3;
static const int CTL_FPGAID_MASK        = 0xFF;

DAQ::DAQ(WishboneInterface* wb) : wb_{wb} {
  n_links=(wb_->wb_read(tgt_DAQ_Control,CTL_CONTROL_REG)&CTL_LINK_COUNT_MASK)>>
      CTL_LINK_COUNT_SHIFT;
}

void DAQ::reset() {
  wb_->wb_write(tgt_DAQ_Control,CTL_CONTROL_REG,CTL_RESET); // hard reset
}

void DAQ::getHeaderOccupancy(int& current, int& maximum) {
  uint32_t val=wb_->wb_read(tgt_DAQ_Control,CTL_OCCUPANCY_REG);
  current=(val>>CTL_OCCUPANCY_CURRENT_SHIFT)&CTL_OCCUPANCY_MASK;
  maximum=(val>>CTL_OCCUPANCY_MAXIMUM_SHIFT)&CTL_OCCUPANCY_MASK;
}

void DAQ::setIds(int fpga_id) {
  wb_->wb_write(tgt_DAQ_Control,CTL_FPGAID_REG,fpga_id&CTL_FPGAID_MASK);
  for (int ilink=0; ilink<n_links; ilink++) {
    wb_->wb_write(tgt_DAQ_LinkFmt,(ilink<<7)|2,(fpga_id<<8)|(ilink));
  }
}

int DAQ::getFPGAid() {
  uint32_t val=wb_->wb_read(tgt_DAQ_Control,CTL_FPGAID_REG);
  return val&CTL_FPGAID_MASK;
}

static const uint32_t CTL_LINK_ENABLE                   = 0x0001;
static const uint32_t CTL_LINK_ZS_ENABLE                = 0x0002;
static const uint32_t CTL_LINK_ZS_FULLSUPPRESS          = 0x0004;

void DAQ::setupLink(int ilink, bool zs, bool zs_all, int l1a_delay, int l1a_capture_width) {
  uint32_t reg1=wb_->wb_read(tgt_DAQ_LinkFmt,(ilink<<7)|1);
  // must disable the link
  bool isEnable=(reg1&CTL_LINK_ENABLE)!=0;
  reg1=reg1&0xFFFFFFFEu;
  wb_->wb_write(tgt_DAQ_LinkFmt,(ilink<<7)|1,reg1);  
  // set the bits for per-link
  reg1=reg1&0xFFFFFFF0u;
  if (zs) reg1|=CTL_LINK_ZS_ENABLE;
  if (zs && zs_all) reg1|=CTL_LINK_ZS_FULLSUPPRESS;
  printf("%d %08x\n",ilink,reg1);
  wb_->wb_write(tgt_DAQ_LinkFmt,(ilink<<7)|1,reg1);

  // set l1a capture setup
  uint32_t regi=wb_->wb_read(tgt_DAQ_Inbuffer,(ilink<<7)|1);
  regi=(regi&0xFF)|((l1a_delay&0xFF)<<8)|((l1a_capture_width&0xFF)<<16);
  wb_->wb_write(tgt_DAQ_Inbuffer,(ilink<<7)|1,regi);
  
  if (isEnable) {
    reg1|=0x1;
    wb_->wb_write(tgt_DAQ_LinkFmt,(ilink<<7)|1,reg1);
  }
}

void DAQ::getLinkSetup(int ilink, bool& zs, bool& zs_all, int& l1a_delay, int& l1a_capture_width) {
  uint32_t reg1;

  reg1=wb_->wb_read(tgt_DAQ_LinkFmt,(ilink<<7)|1);
  zs=(reg1&CTL_LINK_ZS_ENABLE)!=0;
  zs_all=(reg1&CTL_LINK_ZS_FULLSUPPRESS)!=0;
  
  // set l1a capture setup
  reg1=wb_->wb_read(tgt_DAQ_Inbuffer,(ilink<<7)|1);
  l1a_delay=(reg1>>8)&0xFF;
  l1a_capture_width=(reg1>>16)&0xFF;
}


void DAQ::bufferStatus(int ilink, bool postfmt, bool& empty, bool& full) {
  if (postfmt) {
    uint32_t reg2=wb_->wb_read(tgt_DAQ_LinkFmt,(ilink<<7)|2);
    empty=(((reg2>>23)&0x1)!=0);
    full=(((reg2>>22)&0x1)!=0);
  } else {
    uint32_t reg3=wb_->wb_read(tgt_DAQ_Inbuffer,(ilink<<7)|3);
    empty=(((reg3>>27)&0x1)!=0);
    full=(((reg3>>26)&0x1)!=0);
  }
}

void DAQ::enable(bool enable) {
  for (int ilink=0; ilink<n_links; ilink++) {
    uint32_t reg1=wb_->wb_read(tgt_DAQ_LinkFmt,(ilink<<7)|1);
    reg1=reg1&0xFFFFFFFEu;
    if (enable) reg1|=1;
    wb_->wb_write(tgt_DAQ_LinkFmt,(ilink<<7)|1,reg1);
  }
}

bool DAQ::enabled() {
  uint32_t reg1=wb_->wb_read(tgt_DAQ_LinkFmt,1);
  return (reg1&1)!=0;
}

}
