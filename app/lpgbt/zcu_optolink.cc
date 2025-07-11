#include "zcu_optolink.h"

namespace pflib {
namespace zcu {

OptoLink::OptoLink() : transright_("transceiver_right") {

  // enable all SFPs, use internal clock
  transright_.write(0x2,0xF0000);
  
}

static const uint32_t REG_STATUS                 = 3;
  
void OptoLink::reset_link() {
  transright_.write(0x0,0x2); //TX_RESET
  transright_.write(0x0,0x1); //GTH_RESET
  transright_.write(0x0,0x4); //RX_RESET

  int done;
  int attempts=1;
  done=transright_.readMasked(REG_STATUS,0x8);
  
  while (!done and attempts<100) {
    if (attempts % 2) {
      transright_.write(0x0,0x1); //GTH_RESET
      usleep(1000);
      done=transright_.readMasked(REG_STATUS,0x8);
      //      printf("   After %d attempts, BUFFBYPASS_DONE is %d (GTH_RESET)\n",attempts,done);
    } else {
      transright_.write(0x0,0x4); //RX_RESET
      usleep(1000);
      done=transright_.readMasked(REG_STATUS,0x8);
      //      printf("   After %d attempts, BUFFBYPASS_DONE is %d (RX_RESET)\n",attempts,done);
    }
    attempts += 1;
  }

  
}

static const uint32_t REG_POLARITY              = 0x1;
static const uint32_t MASK_POLARITY_RX          = 0x00000FFF;
static const uint32_t MASK_POLARITY_TX          = 0x0FFF0000;

bool OptoLink::get_polarity(int ichan, bool is_rx) {
  return (transright_.readMasked(REG_POLARITY, (is_rx)?(MASK_POLARITY_RX):(MASK_POLARITY_TX))&(1<<ichan))!=0;
}

void OptoLink::set_polarity(bool polarity, int ichan, bool is_rx) {
  int ibit=ichan+(is_rx?(0):(16));
  uint32_t val=transright_.read(REG_POLARITY);
  val=val ^ (1<<ibit);
  transright_.write(REG_POLARITY,val);
}


std::map<std::string, uint32_t> OptoLink::opto_status() {
  std::map<std::string, uint32_t> retval;
  uint32_t val=transright_.read(REG_STATUS);
  retval["TX_RESETDONE"]=(val>>0) & 0x1;
  retval["RX_RESETDONE"]=(val>>1) & 0x1;
  retval["CDR_STABLE"]=(val>>2) & 0x1;
  retval["BUFFBYPASS_DONE"]=(val>>3) & 0x1;
  retval["BUFFBYPASS_ERROR"]=(val>>4) & 0x1;
  retval["CDR_LOCK"]=transright_.read(7)&0xFFF;
  return retval;
}

std::map<std::string, uint32_t> OptoLink::opto_rates() {
  std::map<std::string, uint32_t> retval;

  const char* tnames[]={"S_AXI_ACLK","AXIS_clk","GTH_REFCLK","EXT_REFCLK",
    "RX00","RX01","RX02","RX03","RX04","RX05","RX06","RX07",
    "RX08","RX09","RX10","RX11",0};
  const int TRIGHT_RATES_OFFSET = 16;
  for (int i=0; tnames[i]!=0; i++) 
    retval[tnames[i]]=transright_.read(TRIGHT_RATES_OFFSET+i);

  return retval;
  
}

}
}
