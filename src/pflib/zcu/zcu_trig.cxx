#include "pflib/zcu/zcu_trig.h"

#include "pflib/Exception.h"
#include "pflib/packing/Hex.h"
using pflib::packing::hex;

namespace pflib {
namespace zcu {

static constexpr uint32_t ADDR_RESET = 0x100 / 4;
static constexpr uint32_t MASK_SW_RESET = 0x00000001;
static constexpr uint32_t MASK_SINGLE_SHOT_RESET = 0x00000002;

static constexpr uint32_t ADDR_ADV_BUFFER = 0x080 / 4;
static constexpr uint32_t MASK_ADV_BUFFER = 0x2;
static constexpr uint32_t MASK_ADV_ALGO_BUFFER = 0x4;

static constexpr uint32_t ADDR_CONFIGURE = 0x600 / 4;
static constexpr uint32_t MASK_ENABLE_SINGLE_SHOT = 0x00000002;
static constexpr uint32_t MASK_LINK_CAPTURE_DELAY = 0x0000FFF0;
static constexpr uint32_t MASK_LINK_BX_DELAY_ZERO = 0x00030000;
static constexpr uint32_t MASK_LINK_BX_DELAY = 0x0FFF0000;

static constexpr uint32_t ADDR_PIPELINE_DEPTH = 0x604 / 4;
static constexpr uint32_t MASK_PIPELINE_DEPTH = 0x000000FF;

static constexpr uint32_t ADDR_SAMPLES_PER_L1A = 0x400 / 4;
static constexpr uint32_t MASK_SAMPLES_PER_L1A = 0x00003F00;

static constexpr uint32_t ADDR_PRESAMPLES = 0x400 / 4;
static constexpr uint32_t MASK_PRESAMPLES = 0x000FC000;

static constexpr uint32_t ADDR_ECON_ID = 0x400 / 4;
static constexpr uint32_t MASK_ECON_ID = 0x3FF00000;
static constexpr uint32_t ADDR_ECON_ID_2 = 0x404 / 4;
static constexpr uint32_t MASK_ECON_ID_2 = 0x000003FF;

static constexpr uint32_t ADDR_ALIGNER_SPY_BASE = (0xC00 | 0x100) / 4;

static constexpr uint32_t ADDR_N_ELINKS = 0x800 / 4;
static constexpr uint32_t MASK_N_ELINKS = 0x0000F000;

static constexpr uint32_t ADDR_DAQ_STATUS = 0x800 / 4;
static constexpr uint32_t MASK_DAQ_TVALID = 0x00000100;
static constexpr uint32_t MASK_ALGO_TVALID = 0x00000400;
static constexpr uint32_t MASK_DAQ_TLAST = 0x00000200;
static constexpr uint32_t MASK_ALGO_TLAST = 0x00000800;

static constexpr uint32_t ADDR_DAQ_DATA = 0x804 / 4;
static constexpr uint32_t ADDR_ALGO_DATA = 0x808 / 4;

static constexpr uint32_t ADDR_SINGLE_SHOT_STATUS = 0xC08 / 4;
static constexpr uint32_t MASK_SINGLE_SHOT_FIRED = 0x00010000;
static constexpr uint32_t MASK_SELF_TRIGGER_COUNT = 0x0000ffff;

ZCUtrig::ZCUtrig() : uio_("trigpath-0"), the_log_{logging::get("ZCUtrig-0")} {
  uint16_t fw = (uio_.read(0) & 0xffff);
  // setting up with expected capture parameters
  pflib_log(debug) << "trigpath firwmare block version " << hex(fw);
  if (fw == 0x1) {
    nelinks_ = 3;
  } else {
    nelinks_ = uio_.readMasked(ADDR_N_ELINKS, MASK_N_ELINKS);
  }
  pflib_log(debug) << "n_elinks = " << nelinks_;
}
void ZCUtrig::reset() { uio_.write(ADDR_RESET, MASK_SW_RESET); }

void ZCUtrig::setup_alignment_capture(int delay) {
  uio_.writeMasked(ADDR_CONFIGURE, MASK_LINK_CAPTURE_DELAY, delay & 0xFFF);
}
int ZCUtrig::get_alignment_capture() {
  return uio_.readMasked(ADDR_CONFIGURE, MASK_LINK_CAPTURE_DELAY);
}

std::vector<uint32_t> ZCUtrig::read_capture_buffer(int ilink) {
  static const int N_SAMPLES = 8;
  std::vector<uint32_t> retval(N_SAMPLES, 0);
  for (int i = 0; i < N_SAMPLES; i++) {
    retval[i] = uio_.read(ADDR_ALIGNER_SPY_BASE + (i + ilink * N_SAMPLES));
  }
  return retval;
}

static const int BITS_OF_DELAY = 2;
static const int MASK_OF_DELAY = 0x3;

void ZCUtrig::set_bx_delay(int ilink, int delay) {
  return uio_.writeMasked(ADDR_CONFIGURE,
                          MASK_LINK_BX_DELAY_ZERO << (BITS_OF_DELAY * ilink),
                          delay & MASK_OF_DELAY);
}

int ZCUtrig::get_bx_delay(int ilink) {
  if (ilink < 0 || ilink >= nelinks_) return -1;
  return uio_.readMasked(ADDR_CONFIGURE,
                         MASK_LINK_BX_DELAY_ZERO << (BITS_OF_DELAY * ilink));
}

void ZCUtrig::setup_daq(int pipeline, int econ_id, int samples_per_l1a,
                        int presamples) {
  uio_.writeMasked(ADDR_PIPELINE_DEPTH, MASK_PIPELINE_DEPTH, pipeline);
  uio_.writeMasked(ADDR_SAMPLES_PER_L1A, MASK_SAMPLES_PER_L1A, samples_per_l1a);
  uio_.writeMasked(ADDR_PRESAMPLES, MASK_PRESAMPLES, presamples);
  uio_.writeMasked(ADDR_ECON_ID, MASK_ECON_ID, econ_id);
}

void ZCUtrig::get_daq_setup(int& pipeline, int& econ_id, int& samples_per_l1a,
                            int& presamples) {
  pipeline = uio_.readMasked(ADDR_PIPELINE_DEPTH, MASK_PIPELINE_DEPTH);
  samples_per_l1a = uio_.readMasked(ADDR_SAMPLES_PER_L1A, MASK_SAMPLES_PER_L1A);
  presamples = uio_.readMasked(ADDR_PRESAMPLES, MASK_PRESAMPLES);
  econ_id = uio_.readMasked(ADDR_ECON_ID, MASK_ECON_ID);
}

bool ZCUtrig::is_sample_available() {
  return uio_.readMasked(ADDR_DAQ_STATUS, MASK_DAQ_TVALID) != 0;
}

std::vector<uint32_t> ZCUtrig::read_sample() {
  std::vector<uint32_t> retval;
  if (!is_sample_available()) return retval;
  uint32_t val;
  do {
    val = uio_.read(ADDR_DAQ_STATUS);
    if (!(val & MASK_DAQ_TVALID)) {
      PFEXCEPTION_RAISE("ReadoutException",
                        "ZCU_TRIG got low TVALID before TLAST");
    }
    // get the next word
    retval.push_back(uio_.read(ADDR_DAQ_DATA));
    // advance the pointer (always)
    uio_.write(ADDR_ADV_BUFFER, MASK_ADV_BUFFER);
  } while (!(val & MASK_DAQ_TLAST));
  return retval;
}

static constexpr uint32_t ADDR_HISTORY_VETO_MASK = 0x608 / 4;
static constexpr uint32_t ADDR_THRESHOLDS = 0x60C / 4;

void ZCUtrig::setup_algo(const std::vector<uint32_t>& parameters) {
  /**
   * the current simple trigger has the following parameters
   * 0 -> history veto mask (1 byte)
   * 1 -> threshold for STC0 (1 byte)
   * 2 -> threshold for STC1
   * ... and so on up to index 8 (STC7)
   */
  if (parameters.size() != 9) {
    PFEXCEPTION_RAISE(
        "BadConfig", "Wrong number of parameters for simple trigger algorithm");
  }
  uio_.writeMasked(ADDR_HISTORY_VETO_MASK, 0xff, parameters[0]);
  std::array<uint32_t, 2> thresholds_registers = {0, 0};
  for (std::size_t i{0}; i < 8; i++) {
    int i_reg = i / 4;
    int shift = (i % 4) * 8;
    thresholds_registers[i_reg] |= ((parameters[i + 1] & 0xff) << shift);
  }
  for (int i{0}; i < thresholds_registers.size(); i++) {
    uio_.write(ADDR_THRESHOLDS + i, thresholds_registers[i]);
  }
}

std::vector<uint32_t> ZCUtrig::get_algo_setup() {
  std::vector<uint32_t> parameters(9);
  parameters[0] = uio_.readMasked(ADDR_HISTORY_VETO_MASK, 0xff);
  std::array<uint32_t, 2> thresholds_registers;
  for (int i{0}; i < thresholds_registers.size(); i++) {
    thresholds_registers[i] = uio_.read(ADDR_THRESHOLDS + i);
  }
  for (std::size_t i{0}; i < 8; i++) {
    parameters[i + 1] = ((thresholds_registers[i / 4] >> (8 * (i % 4))) & 0xff);
  }
  return parameters;
}

bool ZCUtrig::is_algo_output_available() {
  return uio_.readMasked(ADDR_DAQ_STATUS, MASK_ALGO_TVALID) != 0;
}

std::vector<uint32_t> ZCUtrig::read_algo_output_sample() {
  std::vector<uint32_t> retval;
  if (!is_algo_output_available()) return retval;
  uint32_t val;
  do {
    val = uio_.read(ADDR_DAQ_STATUS);
    if (!(val & MASK_ALGO_TVALID)) {
      PFEXCEPTION_RAISE("ReadoutException",
                        "ZCUtrig (ALGO DATA) got low TVALID before TLAST");
    }
    // get the next word
    retval.push_back(uio_.read(ADDR_ALGO_DATA));
    // advance the pointer (always)
    uio_.write(ADDR_ADV_BUFFER, MASK_ADV_ALGO_BUFFER);
  } while (!(val & MASK_ALGO_TLAST));
  return retval;
}

bool ZCUtrig::get_enable_single_shot() {
  return uio_.readMasked(ADDR_CONFIGURE, MASK_ENABLE_SINGLE_SHOT) == 1;
}

int ZCUtrig::get_self_trigger_count() {
  return uio_.readMasked(ADDR_SINGLE_SHOT_STATUS, MASK_SELF_TRIGGER_COUNT);
}

void ZCUtrig::enable_single_shot(bool enable) {
  uio_.writeMasked(ADDR_CONFIGURE, MASK_ENABLE_SINGLE_SHOT, enable);
}

bool ZCUtrig::single_shot_fired() {
  return uio_.readMasked(ADDR_SINGLE_SHOT_STATUS, MASK_SINGLE_SHOT_FIRED) == 1;
}

void ZCUtrig::reset_single_shot() {
  uio_.writeMasked(ADDR_RESET, MASK_SINGLE_SHOT_RESET, 1);
}

class FWHisto {
 public:
  FWHisto(UIO& uio, int isub, uint32_t data_reg, uint32_t addr_reg = 0x604 / 4, uint32_t addr_mask = 0x3FF0000)
    : uio_{uio}, ihist_{isub}, regd_{data_reg}, rega_{addr_reg}, maska_{addr_mask} {}
  std::vector<uint32_t> read_histogram() {
    std::vector<uint32_t> data(256, 0);
    for (int i{0}; i < 256; i++) {
      uio_.writeMasked(rega_, maska_, i|(ihist_ << 8));
      data[i] = uio_.read(regd_);
    }
    return data;
  }
 private:
  UIO& uio_;
  int ihist_;
  uint32_t regd_, rega_, maska_;
};

static constexpr uint32_t ADDR_HISTO_CLEAR = 0x100 / 4;
static constexpr uint32_t MASK_HISTO_CLEAR = 0x4;

static constexpr uint32_t ADDR_TRIG_HISTO_03 = 0xC40 / 4;
static constexpr uint32_t ADDR_TRIG_HISTO_47 = 0xC44 / 4;
static constexpr uint32_t ADDR_TRIG_HISTO_TEST = 0xC7C / 4;

void ZCUtrig::clear_histograms() {
  uio_.write(ADDR_HISTO_CLEAR, MASK_HISTO_CLEAR);
}

void ZCUtrig::debug_histogram(int i) {
  uio_.write(ADDR_HISTO_CLEAR, (i & 0xFF) << 8);
  std::array<std::vector<uint32_t>, 4> hists;
  for (int i{0}; i < 4; i++) {
    hists[i] = FWHisto(uio_, i, ADDR_TRIG_HISTO_TEST).read_histogram();
  }
  printf("bin: %10u %10u %10u %10u\n", 0, 1, 2, 3);
  for (int i{0}; i < 256; i++) {
    printf("%3d:", i);
    for (int j{0}; j < 4; j++) {
      printf(" %10u", hists[j][i]);
    }
    printf("\n");
  }
}

std::vector<uint32_t> ZCUtrig::read_histogram(int ihist) {
  int block = ihist/4;
  uint32_t daddr = 0;
  if (block == 0) {
    daddr = ADDR_TRIG_HISTO_03;
  } else if (block == 1) {
    daddr = ADDR_TRIG_HISTO_47;
  } else if (block == 2) {
    daddr = ADDR_TRIG_HISTO_TEST;
  } else {
    // bad
  }
  return FWHisto(uio_, ihist%4, daddr).read_histogram();
}

}  // namespace zcu
}  // namespace pflib
