#include "pflib/zcu/zcu_daq.h"

namespace pflib {
namespace zcu {

static constexpr uint32_t ADDR_IDLE_PATTERN = 0x604 / 4;
static constexpr uint32_t ADDR_HEADER_MARKER = 0x600 / 4;
static constexpr uint32_t MASK_HEADER_MARKER = 0x0001FF00;
static constexpr uint32_t ADDR_ENABLE = 0x600 / 4;
static constexpr uint32_t MASK_ENABLE = 0x00000001;
static constexpr uint32_t ADDR_EVB_CLEAR = 0x100 / 4;
static constexpr uint32_t MASK_EVB_CLEAR = 0x00000001;
static constexpr uint32_t ADDR_ADV_IO = 0x080 / 4;
static constexpr uint32_t MASK_ADV_IO = 0x00000001;
static constexpr uint32_t ADDR_ADV_AXIS = 0x080 / 4;
static constexpr uint32_t MASK_ADV_AXIS = 0x00000002;

static constexpr uint32_t ADDR_PACKET_SETUP = 0x400 / 4;
static constexpr uint32_t MASK_ECON_ID = 0x000003FF;
static constexpr uint32_t MASK_L1A_PER_PACKET = 0x00007C00;
static constexpr uint32_t MASK_SOI = 0x000F8000;
static constexpr uint32_t AXIS_ENABLE = 0x80000000;

static constexpr uint32_t ADDR_UPPER_ADDR = 0x404 / 4;
static constexpr uint32_t MASK_UPPER_ADDR = 0x0000003F;

static constexpr uint32_t ADDR_INFO = 0x800 / 4;
static constexpr uint32_t MASK_IO_NEVENTS = 0x0000007F;
static constexpr uint32_t MASK_IO_SIZE_NEXT = 0x0000FF80;
static constexpr uint32_t MASK_AXIS_NWORDS = 0x1FFF0000;
static constexpr uint32_t MASK_TVALID_DAQ = 0x20000000;
static constexpr uint32_t MASK_TREADY_DAQ = 0x40000000;

static constexpr uint32_t ADDR_PAGED_READ = 0x800 / 4;
static constexpr uint32_t ADDR_BASE_COUNTER = 0x900 / 4;

ZCU_Capture::ZCU_Capture() : DAQ(1), capture_("econd-buffer-0") {
  //    printf("Firmware type and version: %08x %08x
  //    %08x\n",capture_.read(0),capture_.read(ADDR_IDLE_PATTERN),capture_.read(ADDR_HEADER_MARKER));
  // setting up with expected capture parameters
  capture_.write(ADDR_IDLE_PATTERN, 0x1277cc);
  capture_.writeMasked(ADDR_HEADER_MARKER, MASK_HEADER_MARKER,
                       0x1E6);  // 0xAA followed by one bit...
  per_econ_ = true;  // reading from the per-econ buffer, not the AXIS buffer
}
void ZCU_Capture::reset() {
  capture_.write(ADDR_EVB_CLEAR, MASK_EVB_CLEAR);  // auto-clear
}
int ZCU_Capture::getEventOccupancy() {
  capture_.writeMasked(ADDR_UPPER_ADDR, MASK_UPPER_ADDR, 0);  // get on page 0
  if (per_econ_)
    return capture_.readMasked(ADDR_INFO, MASK_IO_NEVENTS) / samples_per_ror();
  else if (capture_.readMasked(ADDR_INFO, MASK_AXIS_NWORDS) != 0)
    return 1;
  else
    return 0;
}

void ZCU_Capture::bufferStatus(int ilink, bool& empty, bool& full) {
  int nevt = getEventOccupancy();
  empty = (nevt == 0);
  full = (nevt == 0x7f);
}
void ZCU_Capture::setup(int econid, int samples_per_ror, int soi) {
  pflib::DAQ::setup(econid, samples_per_ror, soi);
  capture_.writeMasked(ADDR_PACKET_SETUP, MASK_ECON_ID, econid);
  capture_.writeMasked(ADDR_PACKET_SETUP, MASK_L1A_PER_PACKET, samples_per_ror);
  capture_.writeMasked(ADDR_PACKET_SETUP, MASK_SOI, soi);
}
void ZCU_Capture::enable(bool doenable) {
  if (doenable)
    capture_.rmw(ADDR_ENABLE, MASK_ENABLE, MASK_ENABLE);
  else
    capture_.rmw(ADDR_ENABLE, MASK_ENABLE, 0);
  if (!per_econ_ && doenable)
    capture_.rmw(ADDR_PACKET_SETUP, AXIS_ENABLE, AXIS_ENABLE);
  else
    capture_.rmw(ADDR_PACKET_SETUP, AXIS_ENABLE, 0);
  pflib::DAQ::enable(doenable);
}

bool ZCU_Capture::enabled() {
  return capture_.readMasked(ADDR_ENABLE, MASK_ENABLE);
}

std::vector<uint32_t> ZCU_Capture::getLinkData(int ilink) {
  uint32_t words = 0;
  std::vector<uint32_t> retval;
  static const uint32_t UBITS = 0x3F00;
  static const uint32_t LBITS = 0x00FF;

  capture_.writeMasked(ADDR_UPPER_ADDR, MASK_UPPER_ADDR,
                       0);  // must be on basic page

  if (per_econ_)
    words = capture_.readMasked(ADDR_INFO, MASK_IO_SIZE_NEXT);
  else
    words = capture_.readMasked(ADDR_INFO, MASK_AXIS_NWORDS);

  uint32_t iold = 0xFFFFFF;
  for (uint32_t i = 0; i < words; i++) {
    if ((iold & UBITS) != (i & UBITS))  // new upper address block
      if (per_econ_)
        capture_.writeMasked(ADDR_UPPER_ADDR, MASK_UPPER_ADDR, (i >> 8) | 0x04);
      else
        capture_.writeMasked(ADDR_UPPER_ADDR, MASK_UPPER_ADDR, (i >> 8) | 0x20);
    retval.push_back(capture_.read(ADDR_PAGED_READ + (i & LBITS)));
  }
  return retval;
}

void ZCU_Capture::advanceLinkReadPtr() {
  if (per_econ_)
    capture_.write(ADDR_ADV_IO, MASK_ADV_IO);  // auto-clear
  else
    capture_.write(ADDR_ADV_AXIS, MASK_ADV_AXIS);  // auto-clear
}

std::map<std::string, uint32_t> ZCU_Capture::get_debug(uint32_t ask) {
  std::map<std::string, uint32_t> dbg;
  capture_.writeMasked(ADDR_UPPER_ADDR, MASK_UPPER_ADDR, 0);
  static const int stepsize = 1;
  FILE* f = fopen("dump.txt", "w");

  for (int i = 0; i < 0xFFF / 4; i++)
    fprintf(f, "%03x %03x %08x\n", i * 4, i, capture_.read(i));
  fclose(f);

  dbg["COUNT_IDLES"] = capture_.read(ADDR_BASE_COUNTER);
  dbg["COUNT_NONIDLES"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 1);
  dbg["COUNT_STARTS"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 2);
  dbg["COUNT_STOPS"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 3);
  dbg["COUNT_WORDS"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 4);
  dbg["COUNT_IO_ADV"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 5);
  dbg["COUNT_TLAST"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 6);
  dbg["QUICKSPY"] = capture_.read(ADDR_BASE_COUNTER + 0x10);
  dbg["STATE"] = capture_.read(ADDR_BASE_COUNTER + 0x11);
  return dbg;
}

}  // namespace zcu
}  // namespace pflib
