#include "pflib/zcu/zcu_optolink.h"

namespace pflib {
namespace zcu {

OptoLink::OptoLink(const std::string& coder_name)
    : transright_("transceiver_right"),
      coder_(coder_name),
      coder_name_(coder_name) {
  // enable all SFPs, use internal clock
  transright_.write(0x2, 0xF0000);
}

static const uint32_t REG_STATUS = 3;

void OptoLink::reset_link() {
  transright_.write(0x0, 0x2);  // TX_RESET
  transright_.write(0x0, 0x1);  // GTH_RESET
  transright_.write(0x0, 0x4);  // RX_RESET

  int done;
  int attempts = 1;
  done = transright_.readMasked(REG_STATUS, 0x8);

  while (!done and attempts < 100) {
    if (attempts % 2) {
      transright_.write(0x0, 0x1);  // GTH_RESET
      usleep(1000);
      done = transright_.readMasked(REG_STATUS, 0x8);
      //      printf("   After %d attempts, BUFFBYPASS_DONE is %d
      //      (GTH_RESET)\n",attempts,done);
    } else {
      transright_.write(0x0, 0x4);  // RX_RESET
      usleep(1000);
      done = transright_.readMasked(REG_STATUS, 0x8);
      //      printf("   After %d attempts, BUFFBYPASS_DONE is %d
      //      (RX_RESET)\n",attempts,done);
    }
    attempts += 1;
  }

  coder_.write(0, 1);  // reset the DECODER
  usleep(1000);
  coder_.write(65, 0x40000000);  // reset IC
  coder_.write(67, 0x40000000);  // reset EC
  usleep(1000);
  coder_.write(65, 0x00000000);  // reset IC
  coder_.write(67, 0x00000000);  // reset EC
}

static const uint32_t REG_POLARITY = 0x1;
static const uint32_t MASK_POLARITY_RX = 0x00000FFF;
static const uint32_t MASK_POLARITY_TX = 0x0FFF0000;
static const uint32_t REG_DOWNLINK_MODE0 = 48;
static const uint32_t MASK_DOWNLINK_MODE = 0x0000FFFF;

bool OptoLink::get_polarity(int ichan, bool is_rx) {
  return (transright_.readMasked(
              REG_POLARITY, (is_rx) ? (MASK_POLARITY_RX) : (MASK_POLARITY_TX)) &
          (1 << ichan)) != 0;
}

void OptoLink::set_polarity(bool polarity, int ichan, bool is_rx) {
  int ibit = ichan + (is_rx ? (0) : (16));
  uint32_t val = transright_.read(REG_POLARITY);
  val = val ^ (1 << ibit);
  transright_.write(REG_POLARITY, val);
}

std::map<std::string, uint32_t> OptoLink::opto_status() {
  std::map<std::string, uint32_t> retval;
  uint32_t val = transright_.read(REG_STATUS);
  retval["TX_RESETDONE"] = (val >> 0) & 0x1;
  retval["RX_RESETDONE"] = (val >> 1) & 0x1;
  retval["CDR_STABLE"] = (val >> 2) & 0x1;
  retval["BUFFBYPASS_DONE"] = (val >> 3) & 0x1;
  retval["BUFFBYPASS_ERROR"] = (val >> 4) & 0x1;
  retval["CDR_LOCK"] = transright_.read(7) & 0xFFF;

  val = coder_.read(2);
  retval["READY"] = (val >> 0) & 0x1;
  retval["NOT_IN_RESET"] = (val >> 1) & 0x1;
  retval["LINK_ERRORS"] = coder_.read(4) & 0xFFFFFF;

  return retval;
}

std::map<std::string, uint32_t> OptoLink::opto_rates() {
  std::map<std::string, uint32_t> retval;

  const char* tnames[] = {
      "S_AXI_ACLK", "AXIS_clk", "GTH_REFCLK", "EXT_REFCLK", "RX00", "RX01",
      "RX02",       "RX03",     "RX04",       "RX05",       "RX06", "RX07",
      "RX08",       "RX09",     "RX10",       "RX11",       0};
  const int TRIGHT_RATES_OFFSET = 16;
  for (int i = 0; tnames[i] != 0; i++)
    retval[tnames[i]] = transright_.read(TRIGHT_RATES_OFFSET + i);

  if (coder_name_ == "singleLPGBT") {
    const char* cnames[] = {"LINK_WORD", "LINK_ERROR", "LINK_CLOCK", "CLOCK_40",
                            0};
    const int CRATES_OFFSET = 80;
    for (int i = 0; cnames[i] != 0; i++)
      retval[cnames[i]] = coder_.read(CRATES_OFFSET + i);
  } else {
    const char* cnames[] = {"DAQ_LINK_WORD",  "TRIG_LINK_WORD",
                            "DAQ_LINK_ERROR", "TRIG_LINK_ERROR",
                            "DAQ_LINK_CLOCK", "TRIG_LINK_CLOCK",
                            "CLOCK_40",       0};
    const int CRATES_OFFSET = 80;
    for (int i = 0; cnames[i] != 0; i++)
      retval[cnames[i]] = coder_.read(CRATES_OFFSET + i);
  }

  return retval;
}

int OptoLink::get_elink_tx_mode(int i) {
  if (i < 0 || i > 3) return -1;
  return coder_.read(REG_DOWNLINK_MODE0 + i);
}
void OptoLink::set_elink_tx_mode(int i, int mode) {
  if (i < 0 || i > 3) return;
  coder_.write(REG_DOWNLINK_MODE0 + i, mode & MASK_DOWNLINK_MODE);
}

void OptoLink::capture_ec(int mode, std::vector<uint8_t>& tx,
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

void OptoLink::capture_ic(int mode, std::vector<uint8_t>& tx,
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
