#include "pflib/zcu/zcu_optolink.h"

#include "pflib/utility/string_format.h"
using pflib::utility::string_format;

namespace pflib {
namespace zcu {

ZCUOptoLink::ZCUOptoLink(const std::string& coder_name, int ilink, bool isdaq)
    : transright_("transceiver_right"),
      coder_(coder_name),
      coder_name_(coder_name),
      ilink_(ilink),
      isdaq_(isdaq) {
  // enable all SFPs, use internal clock
  transright_.write(0x2, 0xF0000);
  int chipaddr = 0x78;          // EC
  if (isdaq) chipaddr |= 0x04;  // IC

  transport_ =
      std::make_unique<lpGBT_ICEC_Simple>(coder_name, !isdaq, chipaddr);
}

static const uint32_t REG_STATUS = 3;

void ZCUOptoLink::soft_reset_link() {
  /// reset the decoder for the current link
  coder_.write(0, 1 << (ilink_ % 2));
}

void ZCUOptoLink::reset_link() {
  /**
   * This reset actually affects all links (SFPs) in a block (quad)
   * connected to the ZCU.
   *
   * The entire quad shares the same reset and status bits.
   */
  static const int GTH_RESET = 0x1;
  static const int TX_RESET = 0x2;
  static const int RX_RESET = 0x4;
  transright_.write(0x0, TX_RESET);
  transright_.write(0x0, GTH_RESET);
  transright_.write(0x0, RX_RESET);
  usleep(1000);
  int done = transright_.readMasked(REG_STATUS, 0x8);
  int attempts = 1;
  while (!done and attempts < 100) {
    if (attempts % 10 == 0) {
      transright_.write(0x0, GTH_RESET);
      usleep(1000);
      done = transright_.readMasked(REG_STATUS, 0x8);
    } else {
      transright_.write(0x0, RX_RESET);
      usleep(1000);
      done = transright_.readMasked(REG_STATUS, 0x8);
    }
    attempts += 1;
  }

  if (!done) {
    printf("Failed to get BUFFBYPASS_DONE after %d attempts\n", attempts);
    return;
  }

  /**
   * After BUFFBYPASS_DONE, then we reset the decoder
   * (which depends on the link), IC, and EC (which are
   * for a daq/trg link pair.
   */
  soft_reset_link();
  if (isdaq_) {
    usleep(1000);
    coder_.write(65, 0x40000000);  // reset IC
    coder_.write(67, 0x40000000);  // reset EC
    usleep(1000);
    coder_.write(65, 0x00000000);  // reset IC
    coder_.write(67, 0x00000000);  // reset EC
  }
}

void ZCUOptoLink::run_linktrick() {
  transport_->write_reg(0x128, 5);
  sleep(1);
  transport_->write_reg(0x128, 0);
}

static const uint32_t REG_POLARITY = 0x1;
static const uint32_t MASK_POLARITY_RX = 0x00000FFF;
static const uint32_t MASK_POLARITY_TX = 0x0FFF0000;
static const uint32_t REG_DOWNLINK_MODE0 = 48;
static const uint32_t MASK_DOWNLINK_MODE = 0x0000FFFF;

static const int SFP0_OFFSET = 8;  // start with SFP0

bool ZCUOptoLink::get_rx_polarity() {
  return (transright_.readMasked(REG_POLARITY, MASK_POLARITY_RX) &
          (1 << (ilink_ + SFP0_OFFSET))) != 0;
}
bool ZCUOptoLink::get_tx_polarity() {
  return (transright_.readMasked(REG_POLARITY, MASK_POLARITY_TX) &
          (1 << (ilink_ + SFP0_OFFSET))) != 0;
}

void ZCUOptoLink::set_rx_polarity(bool polarity) {
  int ibit = ilink_ + SFP0_OFFSET;
  uint32_t val = transright_.read(REG_POLARITY);
  val = val | (1 << ibit);
  if (!polarity) val = val ^ (1 << ibit);
  transright_.write(REG_POLARITY, val);
}

void ZCUOptoLink::set_tx_polarity(bool polarity) {
  int ibit = ilink_ + SFP0_OFFSET + 16;
  uint32_t val = transright_.read(REG_POLARITY);
  val = val | (1 << ibit);
  if (!polarity) val = val ^ (1 << ibit);
  transright_.write(REG_POLARITY, val);
}

std::map<std::string, uint32_t> ZCUOptoLink::opto_status() {
  std::map<std::string, uint32_t> retval;

  uint32_t val = transright_.read(REG_STATUS);
  retval["TX_RESETDONE"] = (val >> 0) & 0x1;
  retval["RX_RESETDONE"] = (val >> 1) & 0x1;
  retval["CDR_STABLE"] = (val >> 2) & 0x1;
  retval["BUFFBYPASS_DONE"] = (val >> 3) & 0x1;
  retval["BUFFBYPASS_ERROR"] = (val >> 4) & 0x1;
  retval["CDR_LOCK"] = transright_.read(7) & 0xFFF;

  val = coder_.read(2);
  std::string prefix = "LINK" + std::to_string(ilink_);
  retval[prefix + " READY"] = (val >> (0 * 2 + (ilink_ % 2))) & 0x1;
  retval[prefix + " NOT_IN_RESET"] = (val >> (1 * 2 + (ilink_ % 2))) & 0x1;
  retval[prefix + " LINK_ERRORS"] = coder_.read(4 + (ilink_ % 2)) & 0xFFFFFF;

  for (int i{0}; i < 80; i++) {
    printf("CODER @ %02d: %04x\n", i, coder_.read(i));
  }

  return retval;
}

std::map<std::string, uint32_t> ZCUOptoLink::opto_rates() {
  std::map<std::string, uint32_t> retval;

  static const std::array<const char*, 4> tnames = {"S_AXI_ACLK", "AXIS_clk",
                                                    "GTH_REFCLK", "EXT_REFCLK"};
  static const int TRIGHT_RATES_OFFSET = 0x10;
  for (std::size_t i{0}; i < tnames.size(); i++) {
    retval[tnames[i]] = transright_.read(TRIGHT_RATES_OFFSET + i);
  }

  retval["RX-LINK"] =
      transright_.read(TRIGHT_RATES_OFFSET + 4 + SFP0_OFFSET + ilink_);

  if (coder_name_ == "singleLPGBT") {
    static const std::array<const char*, 4> cnames = {"LINK_WORD", "LINK_ERROR",
                                                      "LINK_CLOCK", "CLOCK_40"};
    const int CRATES_OFFSET = 80;
    for (int i = 0; i < cnames.size(); i++) {
      retval[cnames[i]] = coder_.read(CRATES_OFFSET + i);
    }
  } else {
    static const std::array<const char*, 7> cnames = {
        "DAQ_LINK_WORD",   "TRIG_LINK_WORD", "DAQ_LINK_ERROR",
        "TRIG_LINK_ERROR", "DAQ_LINK_CLOCK", "TRIG_LINK_CLOCK",
        "CLOCK_40"};
    const int CRATES_OFFSET = 80;
    for (int i = 0; i < cnames.size(); i++) {
      retval[cnames[i]] = coder_.read(CRATES_OFFSET + i);
    }
  }

  return retval;
}

int ZCUOptoLink::get_elink_tx_mode(int elink) {
  if (elink < 0 || elink > 3 || !isdaq_) return -1;
  return coder_.read(REG_DOWNLINK_MODE0 + elink);
}

void ZCUOptoLink::set_elink_tx_mode(int elink, int mode) {
  if (elink < 0 || elink > 3 || !isdaq_) return;
  coder_.write(REG_DOWNLINK_MODE0 + elink, mode & MASK_DOWNLINK_MODE);
}

void ZCUOptoLink::capture_ec(int mode, std::vector<uint8_t>& tx,
                             std::vector<uint8_t>& rx) {
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
}

void ZCUOptoLink::capture_ic(int mode, std::vector<uint8_t>& tx,
                             std::vector<uint8_t>& rx) {
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
}

}  // namespace zcu
}  // namespace pflib
