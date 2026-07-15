#include "pflib/zcu/zcu_trig.h"

#include "pflib/Exception.h"

namespace pflib {
namespace zcu {

static constexpr uint32_t ADDR_SW_RESET = 0x100 / 4;
static constexpr uint32_t MASK_SW_RESET = 0x1;

static constexpr uint32_t ADDR_ADV_BUFFER = 0x080 / 4;
static constexpr uint32_t MASK_ADV_BUFFER = 0x2;

static constexpr uint32_t ADDR_LINK_CAPTURE_DELAY = 0x600 / 4;
static constexpr uint32_t MASK_LINK_CAPTURE_DELAY = 0x0000FFF0;

static constexpr uint32_t ADDR_LINK_BX_DELAY = 0x600 / 4;
static constexpr uint32_t MASK_LINK_BX_DELAY = 0x0FFF0000;
static constexpr uint32_t MASK_LINK_BX_DELAY_ZERO = 0x00030000;

static constexpr uint32_t ADDR_PIPELINE_DEPTH = 0x604 / 4;
static constexpr uint32_t MASK_PIPELINE_DEPTH = 0x000000FF;

static constexpr uint32_t ADDR_SAMPLES_PER_L1A = 0x400 / 4;
static constexpr uint32_t MASK_SAMPLES_PER_L1A = 0x00003F00;

static constexpr uint32_t ADDR_PRESAMPLES = 0x400 / 4;
static constexpr uint32_t MASK_PRESAMPLES = 0x000FC000;

static constexpr uint32_t ADDR_ECON_ID = 0x400 / 4;
static constexpr uint32_t MASK_ECON_ID = 0x3FF00000;

static constexpr uint32_t ADDR_ALIGNER_SPY_BASE = (0xC00 | 0x100) / 4;

static constexpr uint32_t ADDR_N_ELINKS = 0x800 / 4;
static constexpr uint32_t MASK_N_ELINKS = 0x0000F000;

static constexpr uint32_t ADDR_DAQ_TVALID = 0x800 / 4;
static constexpr uint32_t MASK_DAQ_TVALID = 0x00000100;
static constexpr uint32_t ADDR_DAQ_TLAST = 0x800 / 4;
static constexpr uint32_t MASK_DAQ_TLAST = 0x00000200;

static constexpr uint32_t ADDR_DAQ_DATA = 0x804 / 4;

ZCUtrig::ZCUtrig() : uio_("trigpath-0") {
  uint32_t fw = uio_.read(0);
  //    printf("Firmware type and version: %08x %08x
  //    %08x\n",uio_.read(0),uio_.read(ADDR_IDLE_PATTERN),uio_.read(ADDR_HEADER_MARKER));
  // setting up with expected capture parameters
  if ((fw & 0xFFFF) == 0x1)
    nelinks_ = 3;
  else {
    nelinks_ = uio_.readMasked(ADDR_N_ELINKS, MASK_N_ELINKS);
  }
}
void ZCUtrig::reset() { uio_.write(ADDR_SW_RESET, ADDR_SW_RESET); }

void ZCUtrig::setup_alignment_capture(int delay) {
  uio_.writeMasked(ADDR_LINK_CAPTURE_DELAY, MASK_LINK_CAPTURE_DELAY,
                   delay & 0xFFF);
}
int ZCUtrig::get_alignment_capture() {
  return uio_.readMasked(ADDR_LINK_CAPTURE_DELAY, MASK_LINK_CAPTURE_DELAY);
}

std::vector<uint32_t> ZCUtrig::read_capture_buffer(int ilink) {
  static const int N_SAMPLES = 8;
  std::vector<uint32_t> retval(N_SAMPLES, 0);
  for (int i = 0; i < N_SAMPLES; i++) {
    retval[i] = uio_.read(ADDR_ALIGNER_SPY_BASE + (i + ilink * N_SAMPLES));
    //    printf("%04x
    //    %08x\n",ADDR_ALIGNER_SPY_BASE+(i+ilink*N_SAMPLES),retval[i]);
  }
  return retval;
}

static const int BITS_OF_DELAY = 2;
static const int MASK_OF_DELAY = 0x3;

void ZCUtrig::set_bx_delay(int ilink, int delay) {
  return uio_.writeMasked(ADDR_LINK_CAPTURE_DELAY,
                          MASK_LINK_BX_DELAY_ZERO << (BITS_OF_DELAY * ilink),
                          delay & MASK_OF_DELAY);
}

int ZCUtrig::get_bx_delay(int ilink) {
  if (ilink < 0 || ilink >= nelinks_) return -1;
  return uio_.readMasked(ADDR_LINK_CAPTURE_DELAY,
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

bool ZCUtrig::is_event_available() {
  return uio_.readMasked(ADDR_DAQ_TVALID, MASK_DAQ_TVALID) != 0;
}

std::vector<uint32_t> ZCUtrig::read_event() {
  std::vector<uint32_t> retval;
  if (!is_event_available()) return retval;
  uint32_t val;
  do {
    val = uio_.read(ADDR_DAQ_TVALID);
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

}  // namespace zcu
}  // namespace pflib
