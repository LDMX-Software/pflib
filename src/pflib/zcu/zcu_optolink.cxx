#include "pflib/zcu/zcu_optolink.h"

#include "pflib/utility/string_format.h"
using pflib::utility::string_format;

#include "pflib/packing/Hex.h"
using pflib::packing::hex;

namespace pflib {
namespace zcu {

/// construct the trigpath-N coder name from the some-N other name
std::string trigpath_coder_name(const std::string& coder_name) {
  /// assume the suffix "-N" is present in provided name
  auto hyphen_it = coder_name.find("-");
  if (hyphen_it == std::string::npos) {
    /// don't return a suffix if we didn't find one
    return "trigpath";
  }
  return "trigpath"+coder_name.substr(hyphen_it);
}

ZCUOptoLink::ZCUOptoLink(const std::string& coder_name, int ilink, bool isdaq)
    : transright_("transceiver_right"),
      coder_(isdaq ? coder_name : trigpath_coder_name(coder_name)),
      ilink_(ilink),
      isdaq_(isdaq),
      the_log_{logging::get("zcu_optolink")} {
  /**
   * The actual UIO device we connect to depends on if we are connecting
   * to the DATA path or the TRIG path.
   * In the final system, the DATA and TRIG path firmwares live on different
   * chips, so we mimic that here by having the DATA and TRIG path firmwares
   * at least reside in different blocks.
   * 'standardLpGBTpair-N' -> slow control and DATA path for pair N (0 or 1)
   * 'trigpath-N' -> TRIG path for pair N
   * So if this opto link is labeled as TRIG (isdaq == false), then we
   * use trigpath-N instead of the provided coder_name.
   * However, we still use the provided coder_name for the I2C transport
   * since the slow control for the TRIG path chips will still proceed
   * via the standardLpGBTpair-N firmware.
   *
   * @note The provided coder_name can be something besides standardLpGBTpair-N
   * when we are testing the LpGBT mezzanine with a special ZCU connector
   * and firmware.
   */
  uint16_t olink_block_vers = (coder_.read(isdaq ? 256 : 0) & 0xffff);
  pflib_log(info) << "OLink FW Vers: " << hex(olink_block_vers);
  if (isdaq and olink_block_vers < 0x5000) {
    pflib_log(error) << "Newer software requires " << coder_name
                     << " FW verion at least 0x5000 due to separation"
                     << " of TRIG and DATA paths, update firmware.";
  }
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
  if (isdaq_) {
    static const uint32_t DECODER_RESET_REG = 0;
    coder_.write(DECODER_RESET_REG, 1);
  } else {
    static const uint32_t DECODER_RESET_ADDR = 0x100 / 4;
    static const uint32_t DECODER_RESET_MASK = (1 << 4);
    coder_.writeMasked(DECODER_RESET_ADDR, DECODER_RESET_MASK, 1);
  }
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
  while (!done and attempts < 1000) {
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
   * for a daq/trg link pair).
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

  std::string prefix = "LINK" + std::to_string(ilink_);
  if (isdaq_) {
    static const uint32_t LINK_STATUS_REG = 2;
    val = coder_.read(LINK_STATUS_REG);
    // READY is lpgbt_decoder_read
    retval[prefix + " READY"] = (val >> 0) & 0x1;
    // NOT_IN_RESET is resetn_decoder_gth_clock
    retval[prefix + " NOT_IN_RESET"] = (val >> 1) & 0x1;
    /**
     * The LINK_ERRORS are not being incremented when a known error is injected,
     * so without a future firmware patch, we are electing to ignore them.
    retval[prefix + " LINK_ERRORS"] = coder_.read(4 + (ilink_ % 2)) & 0xFFFFFF;
     */
  } else {
    static const uint32_t LINK_STATUS_REG = 0xC04 / 4;
    val = coder_.read(LINK_STATUS_REG);
    retval[prefix + " READY"] = (val >> 31) & 0x1;
    retval[prefix + " NOT_IN_RESET"] = (val >> 30) & 0x1;
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

  if (coder_.name() == "singleLPGBT") {
    static const std::array<const char*, 4> cnames = {"LINK_WORD", "LINK_ERROR",
                                                      "LINK_CLOCK", "CLOCK_40"};
    const int CRATES_OFFSET = 80;
    for (int i = 0; i < cnames.size(); i++) {
      retval[cnames[i]] = coder_.read(CRATES_OFFSET + i);
    }
  } else if (isdaq_) {
    static const std::array<const char*, 6> cnames = {
        "LINK_WORD", "LINK_ERROR", "LINK_CLOCK",
        "CLOCK_40", "AXI_CLK", "LINK_FECERR"
        };
    const int CRATES_OFFSET = 80;
    for (int i = 0; i < cnames.size(); i++) {
      uint32_t val = coder_.read(CRATES_OFFSET + i);
      if (i == 5) {
        // the FECERR rates have a longer integration time
        val /= 1000;
      }
      retval[cnames[i]] = val;
    }
  } else {
    // is trigger link and not singleLPGBT
    static constexpr int RATES_OFFSET = (0xC00 + 4*0x20) / 4;
    // same names but ordered differently in registers
    static const std::array<const char*, 6> cnames = {
      "AXI_CLK", "CLOCK_40", "LINK_CLOCK",
      "LINK_WORD", "LINK_ERROR", "LINK_FECERR"
    };
    for (int i{0}; i < cnames.size(); i++) {
      uint32_t val = coder_.read(RATES_OFFSET + i);
      if (i == 5) {
        // the FECERR rates have a longer integration time
        val /= 1000;
      }
      retval[cnames[i]] = val;
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
