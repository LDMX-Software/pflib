#include "pflib/bittware/bittware_optolink.h"
namespace pflib {
namespace bittware {

static constexpr uint32_t GTY_QUAD_BASE_ADDRESS = 0x2000; // compiled into the firmware
static constexpr uint32_t QUAD_CODER0_BASE_ADDRESS = 0x3000; // compiled into the firmware

BWOptoLink::BWOptoLink(int ilink)
    : gtys_(GTY_QUAD_BASE_ADDRESS,0xFFF),
      ilink_(ilink),
      isdaq_(true)
{
  coder_=std::make_shared<AxiLite>(QUAD_CODER0_BASE_ADDRESS,0xFFF);
  icecoder=coder_;

  int chipaddr = 0x78;          // EC
  if (isdaq_) chipaddr |= 0x04;  // IC

  transport_=std::make_unique<BWlpGBT_Transport>(*iceccoder_,ilink,chipaddr,isdaq_);
}

BWOptoLink::BWOptoLink(int ilink, BWOptoLink& daqlink)
    : gtys_(GTY_QUAD_BASE_ADDRESS,0xFFF),
      ilink_(ilink),
      isdaq_(false)
{
  coder_=std::make_shared<AxiLite>(QUAD_CODER0_BASE_ADDRESS,0xFFF);
  iceccoder_=daqlink.iceccoder_;
  
  int chipaddr = 0x78;          // EC
  if (isdaq_) chipaddr |= 0x04;  // IC

  transport_=std::make_unique<BWlpGBT_Transport>(iceccoder_,ilink,chipaddr,isdaq_);
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

static constexpr int REG_ADDR_TX_DATA = 0x604;  
static constexpr int REG_CTL_N_READ   = 0x608;
static constexpr uint32_t MASK_N_READ = 0x000000FF;


static constexpr int BIT_START_READ  = 3;
static constexpr int BIT_START_WRITE = 2;
static constexpr int BIT_ADV_WRITE   = 4;
static constexpr int BIT_ADV_READ    = 5;
static constexpr uint32_t MASK_RX_DATA  = 0x00FF;
static constexpr uint32_t MASK_RX_EMPTY = 0x0100;
static constexpr uint32_t MASK_TX_EMPTY = 0x0200;

BWlpGBT_Transport::BWlpGBT_Transport(AxiLite& coder, int ilink, int chipaddr, bool isic) : transport_{coder}, ilink_{ilink}, chipaddr_{chipaddr}, isic_{isic} {
  ctloffset_=16*ilink+(isic_)?(0):(8);
  stsreg_=0xC08+4*ilink;
  stsmask_=(isic_)?(0xFFFF):(0xFFFF0000u);
  pulsereg_=(0x104)+(ilink/2)*4;
  pulseshift_=(ilink%2)*16+(isic_)?(0):(8);
  transport_.write(ctloffset_+REG_CTL_N_READ,0); // choose internal operation, disable spies, etc
}

uint8_t BWlpGBT_Transport::read_reg(uint16_t reg) {
  std::vector<uint8_t> retval = read_regs(reg, 1);
  return retval[0];
}

std::vector<uint8_t> BWlpGBT_Transport::read_regs(uint16_t reg, int n) {
  std::vector<uint8_t> retval;
  int wc = 0;
  
  if (n > 8) {
    retval = read_regs(reg, 8);
    std::vector<uint8_t> later = read_regs(reg + 8, n - 8);
    retval.insert(retval.end(), later.begin(), later.end());
    return retval;
  }

  uint32_t val;
  // set addresses
  val = (uint32_t(reg) << 16) | (uint32_t(chipaddr_) << 8);
  transport_.write(ctloffset_ + REG_ADDR_TX_DATA, val);
  // set length and start
  val = n;
  transport_.writeMasked(ctloffset_ + REG_CTL_N_READ, MASK_N_READ, val);
  // start
  transport_.write(pulsereg_,1<<(BIT_START_READ+pulseshift_));

  // wait for done...
  int timeout = 1000;
  for (val = transport_.readMasked(stsreg_,stsmask_);
       (val & MASK_RX_EMPTY);
       val = transport_.readMasked(stsreg_,stsmask_)) {
    usleep(1);
    timeout--;
    if (timeout == 0) {
      char message[256];
      snprintf(message, 256, "Read register 0x%x timeout", reg);
      PFEXCEPTION_RAISE("ICEC_Timeout", message);
    }
  }
  wc = 0;
  while (!(val & MASK_RX_EMPTY)) {
    uint8_t abyte = uint8_t(val & MASK_RX_DATA);
    transport_.write(pulsereg_,1<<(BIT_ADV_READ+pulseshift_));
    if (wc >= 6 && int(retval.size()) < n) retval.push_back(abyte);
    wc++;
    val = transport_.readMasked(stsreg_,stsmask_);    
  }

  return retval;
}

void BWlpGBT_Transport::write_reg(uint16_t reg, uint8_t value) {
  std::vector<uint8_t> vv(1, value);
  write_regs(reg, vv);
}

void BWlpGBT_Transport::write_regs(uint16_t reg,
                                   const std::vector<uint8_t>& value) {
  size_t wc = 0;

  uint32_t baseval;
  // set addresses
  baseval = (uint32_t(reg) << 16) | (uint32_t(chipaddr_) << 8);

  int ic = 0;
  while (wc < value.size()) {
    transport_.write(ctloffset_ + REG_ADDR_TX_DATA, baseval | uint32_t(value[wc]));
    wc++;
    ic++;
    transport_.write(pulsereg_,1<<(BIT_ADV_WRITE+pulseshift_));
    if (ic == 8 || wc == value.size()) {
      transport_.write(pulsereg_,1<<(BIT_START_WRITE+pulseshift_));
      // wait for tx to be done
      int timeout = 1000;

      for (uint32_t val = transport_.readMasked(stsreg_,stsmask_);
           (val & MASK_TX_EMPTY);
           val = transport_.readMasked(stsreg_,stsmask_)) {
        //	printf("%02x\n",val);
        usleep(1);
        timeout--;
        if (timeout == 0) {
          char message[256];
          snprintf(message, 256, "Write register 0x%x (+%d) timeout (%x)", reg,
                   ic, lpgbt_i2c_addr_);
          PFEXCEPTION_RAISE("ICEC_Timeout", message);
        }
      }
      ic = 0;
    }
  }
}


}  // namespace bittware
}  // namespace pflib
