#include "pflib/bittware/bittware_optolink.h"
namespace pflib {
namespace bittware {

static constexpr uint32_t GTY_QUAD_BASE_ADDRESS = 0x2000; // compiled into the firmware
static constexpr uint32_t QUAD_CODER0_BASE_ADDRESS = 0x3000; // compiled into the firmware

BWOptoLink::BWOptoLink(int ilink, bool isdaq)
    : gtys_(GTY_QUAD_BASE_ADDRESS,0xFFF),
      ilink_(ilink),
      isdaq_(isdaq)
{
  coder_=std::make_unique<AxiLite>(QUAD_CODER0_BASE_ADDRESS,0xFFF);
  transport_=std::make_unique<BWlpGBT_Transport>(*coder_,ilink);
  /*
  int chipaddr = 0x78;          // EC
  if (isdaq) chipaddr |= 0x04;  // IC

  transport_ =
      std::make_unique<lpGBT_ICEC_Simple>(coder_name, !isdaq, chipaddr);
  */
}

void BWOptoLink::reset_link() {  // actually affects all links in a block
  if (!isdaq_) return; // only do these items for DAQ links for now
  gtys_.write(0x080, 0x2); // TX_RESET
  gtys_.write(0x080, 0x1); // GTH_RESET
  gtys_.write(0x080, 0x4); // RX_RESET

  uint32_t REG_STATUS = 0x804 + ilink_*4; 
  
  int done;
  int attempts = 1;
  done = gtys_.readMasked(REG_STATUS, 0x1);

  while (!done and attempts < 100) {
    if (attempts % 2) {
      gtys_.write(0x080, 0x1);  // GTH_RESET
      usleep(1000);
      done = gtys_.readMasked(REG_STATUS, 0x1);
      /*
        printf("   After %d attempts, BUFFBYPASS_DONE is %d (GTH_RESET)\n",
        attempts,done);
      */
    } else {
      gtys_.write(0x080, 0x4);  // RX_RESET
      usleep(1000);
      done = gtys_.readMasked(REG_STATUS, 0x1);
      /*
        printf("   After %d attempts, BUFFBYPASS_DONE is %d (RX_RESET)\n",
        attempts,done);
      */
    }
    attempts += 1;
  }

  uint32_t REG_DECODER_RESET=0x100;
 
  coder_->write(REG_DECODER_RESET, 1<<ilink_);  // reset the DECODER
  uint32_t REG_ICEC_RESET=0x104+(ilink_/2)*4;
  int BIT_IC_RESET=(ilink_%2)?(16+6):(6);
  int BIT_EC_RESET=(ilink_%2)?(16+6+8):(6+8);
  usleep(1000);
  coder_->setclear(REG_ICEC_RESET,BIT_IC_RESET,true); // self clearing
  coder_->setclear(REG_ICEC_RESET,BIT_EC_RESET,true); // self clearing
  usleep(1000);
}

void BWOptoLink::run_linktrick() {
  lpgbt_transport().write_reg(0x128, 5);
  sleep(1);
  lpgbt_transport().write_reg(0x128, 0);
}

static const uint32_t REG_POLARITY = 0x400;
static const uint32_t MASK_POLARITY_RX = 0x0000000F;
static const uint32_t MASK_POLARITY_TX = 0x000F0000;

bool BWOptoLink::get_rx_polarity() {
  return (gtys_.readMasked(REG_POLARITY, MASK_POLARITY_RX) &
          (1 << (ilink_))) != 0;
}
bool BWOptoLink::get_tx_polarity() {
  return (gtys_.readMasked(REG_POLARITY, MASK_POLARITY_TX) &
          (1 << (ilink_))) != 0;
}

void BWOptoLink::set_rx_polarity(bool polarity) {
  int ibit = ilink_;
  uint32_t val = gtys_.read(REG_POLARITY);
  val = val | (1 << ibit);
  if (!polarity) val = val ^ (1 << ibit);
  gtys_.write(REG_POLARITY, val);
}

void BWOptoLink::set_tx_polarity(bool polarity) {
  int ibit = ilink_ + 16;
  uint32_t val = gtys_.read(REG_POLARITY);
  val = val | (1 << ibit);
  if (!polarity) val = val ^ (1 << ibit);
  gtys_.write(REG_POLARITY, val);
}

std::map<std::string, uint32_t> BWOptoLink::opto_status() {
  std::map<std::string, uint32_t> retval;
  uint32_t REG_STATUS = 0x804 + ilink_*4; 
  uint32_t val = gtys_.read(REG_STATUS);
  retval["TX_RESETDONE"] = (val >> 3) & 0x1;
  retval["RX_RESETDONE"] = (val >> 4) & 0x1;
  retval["CDR_STABLE"] = (val >> 2) & 0x1;
  retval["BUFFBYPASS_DONE"] = (val >> 0) & 0x1;
  retval["BUFFBYPASS_ERROR"] = (val >> 1) & 0x1;

  
  val = coder_->read(0xC00);
  retval["READY"] = (val >> ilink_) & 0x1;
  //  retval["LINK_ERRORS"] = coder_.read(4) & 0xFFFFFF;
    
  return retval;
}


static constexpr uint32_t REG_GTYS_RATES = 0x900;

std::map<std::string, uint32_t> BWOptoLink::opto_rates() {
  std::map<std::string, uint32_t> retval;

  /*
  const char* tnames[] = {
    "S_AXI_ACLK", "AXIS_clk", "GTH_REFCLK", "EXT_REFCLK", "RX00", "RX01",
    "RX02",       "RX03",     "RX04",       "RX05",       "RX06", "RX07",
    "RX08",       "RX09",     "RX10",       "RX11",       0};
  */
  const char* tnames[] = {"S_AXI_ACLK", "LCLS2_CLK", "BX_CLK", "TX_CLK",
    "REF_CLK", 0};
  const int twhich[] = {0, 1, 2, 3, 4, -1};

  for (int i = 0; tnames[i] != 0; i++)
    retval[tnames[i]] = gtys_.read(REG_GTYS_RATES + 4*twhich[i])/1000.0;

  retval["RX-LINK"] =
      gtys_.read(REG_GTYS_RATES + 0x14 + 4*ilink_)/1000.0;


  retval["LINK_WORD"]=coder_->read(0x900+ilink_*8)/1000.0;
  retval["LINK_ERROR"]=coder_->read(0x904+ilink_*8)/1000.0;
  
  return retval;
}

int BWOptoLink::get_elink_tx_mode(int elink) {
  if (elink < 0 || elink > 3 || !isdaq_) return -1;
}
void BWOptoLink::set_elink_tx_mode(int elink, int mode) {
  if (elink < 0 || elink > 3 || !isdaq_) return;
}

void BWOptoLink::capture_ec(int mode, std::vector<uint8_t>& tx,
                             std::vector<uint8_t>& rx) {
  /*
  static constexpr int REG_SPY_CTL = 67;
  static constexpr int REG_READ = 69;

  uint32_t val = coder_.read(REG_SPY_CTL);
  val = val & 0x1FF;
  val = val | ((mode & 0x3) << 10) | (1 << 9);
  coder_.write(REG_SPY_CTL, val);

  // now wait for it...
  bool done = false;
  do {
    usleep(100);
    done = (coder_.read(REG_READ) & 0x400) != 0;
    if (mode == 0) done = true;
  } while (!done);
  val = val & 0x1FF;
  val = val | ((mode & 0x3) << 10);  // disable the spy
  coder_.write(REG_SPY_CTL, val);

  tx.clear();
  rx.clear();

  val = val & 0x1FF;
  val = val | ((mode & 0x3) << 10);
  for (int i = 0; i < 256; i++) {
    val = val & 0xFFF;
    val = val | (i << 12);
    coder_.write(REG_SPY_CTL, val);
    usleep(1);
    uint32_t k = coder_.read(REG_READ);
    tx.push_back((k >> (2 + 11)) & 0x3);
    rx.push_back((k >> 11) & 0x3);
  }
  */
}

void BWOptoLink::capture_ic(int mode, std::vector<uint8_t>& tx,
                             std::vector<uint8_t>& rx) {
  /*
  static constexpr int REG_SPY_CTL = 65;
  static constexpr int REG_READ = 68;

  uint32_t val = coder_.read(REG_SPY_CTL);
  val = val & 0x1FF;
  val = val | ((mode & 0x3) << 10) | (1 << 9);
  coder_.write(REG_SPY_CTL, val);

  // now wait for it...
  bool done = false;
  do {
    usleep(100);
    done = (coder_.read(REG_READ) & 0x400) != 0;
    if (mode == 0) done = true;
  } while (!done);
  val = val & 0x1FF;
  val = val | ((mode & 0x3) << 10);  // disable the spy
  coder_.write(REG_SPY_CTL, val);

  tx.clear();
  rx.clear();

  val = val & 0x1FF;
  val = val | ((mode & 0x3) << 10);
  for (int i = 0; i < 256; i++) {
    val = val & 0xFFF;
    val = val | (i << 12);
    coder_.write(REG_SPY_CTL, val);
    usleep(1);
    uint32_t k = coder_.read(REG_READ);
    tx.push_back((k >> (2 + 11)) & 0x3);
    rx.push_back((k >> 11) & 0x3);
  }
  */
}

}  // namespace bittware
}  // namespace pflib
