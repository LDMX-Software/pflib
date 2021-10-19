#include "pflib/DAQ.h"

namespace pflib {

static const int CTL_OCCUPANCY_REG       = 4;
static const uint32_t CTL_OCCUPANCY_MASK = 0x3F;
static const int CTL_OCCUPANCY_CURRENT_SHIFT = 8;
static const int CTL_OCCUPANCY_MAXIMUM_SHIFT = 0;
static const int CTL_FPGAID_REG         = 3;
static const int CTL_

void DAQ::reset() {
  wb_->wb_write(tgt_DAQ_Control,1,1<<29); // hard reset
}

void DAQ::getHeaderOccupancy(int& current, int& maximum) {
  uint32_t val=wb_read(tgt_DAQ_Control,CTL_OCCUPANCY_REG);
  current=(val>>CTL_OCCUPANCY_CURRENT_SHIFT)&CTL_OCCUPANCY_MASK;
  maximum=(val>>CTL_OCCUPANCY_MAXIMUM_SHIFT)&CTL_OCCUPANCY_MASK;
}

void DAQ::setIds(int fpga_id) {
  
}

// Get the FPGA id
  int getFPGAid();
  // Setup a link
  void setupLink(int ilink, bool zs, bool zs_all, int l1a_delay, int l1a_capture_width);
  // 
  void getLinkSetup(int ilink, bool& zs, bool& zs_all, int& l1a_delay, int& l1a_capture_width);
  // get empty/full status for the given link and stage
  void bufferStatus(int ilink, bool postfmt, bool& empty, bool& full);
  // enable/disable the readout
  void enable(bool enable=true);

}
