#include "pflib/bittware/bittware_daq.h"

#include "pflib/packing/Hex.h"
#include "pflib/utility/string_format.h"

namespace pflib {
namespace bittware {

using packing::hex;

static constexpr uint32_t BASE_ADDRESS_CAPTURE0 = 0x8000;

static constexpr uint32_t ADDR_IDLE_PATTERN = 0x604;
static constexpr uint32_t ADDR_HEADER_MARKER = 0x600;
static constexpr uint32_t MASK_HEADER_MARKER_OLD = 0x0001FF00;
static constexpr uint32_t MASK_HEADER_MARKER = 0x01FF0000;
static constexpr uint32_t ADDR_ENABLE = 0x600;
static constexpr uint32_t MASK_ENABLE = 0x0000FFFF;
static constexpr uint32_t ADDR_EVB_CLEAR = 0x100;
static constexpr uint32_t MASK_EVB_CLEAR = 0x00000001;
static constexpr uint32_t ADDR_ADV_IO = 0x080;
// static constexpr uint32_t MASK_ADV_IO = 0x00000001;

static constexpr uint32_t ADDR_DISABLE_AXIS = 0x400;
static constexpr uint32_t MASK_DISABLE_AXIS = 0x00000001;
static constexpr uint32_t ADDR_PACKET_SETUP = 0x400;
static constexpr uint32_t MASK_L1A_PER_PACKET = 0x000001F0;
static constexpr uint32_t MASK_SOI = 0x00001E00;
static constexpr uint32_t ADDR_PICK_ECON = 0x400;
static constexpr uint32_t MASK_PICK_ECON = 0x000F0000;

static constexpr uint32_t ADDR_ECON0_ID = 0x408;  //
static constexpr uint32_t MASK_ECON0_ID = 0x03FF;
static constexpr uint32_t MASK_ECON1_ID = 0x03FF0000;

static constexpr uint32_t ADDR_PAGE_SPY = 0x400;
static constexpr uint32_t MASK_PAGE_SPY = 0x00700000;

static constexpr uint32_t ADDR_SPY_BASE = 0xA00;  // 0xA00 -> 0xBFC

static constexpr uint32_t ADDR_NECONS = 0x800;
static constexpr uint32_t MASK_NECONS = 0x7F000000;
static constexpr uint32_t ADDR_BASE_COUNTER = 0x840;
static constexpr uint32_t ADDR_BASE_LINK_COUNTER = 0x900;
static constexpr uint32_t ADDR_INFO = 0x940;

static constexpr uint32_t MASK_IO_NEVENTS = 0x0000007F;
static constexpr uint32_t MASK_IO_SIZE_NEXT = 0x0000FF80;

BWdaq::BWdaq(const char* dev)
    : capture_(BASE_ADDRESS_CAPTURE0, dev),
      DAQ(capture_.readMasked(ADDR_NECONS,MASK_NECONS)),
      the_log_{logging::get("bw_capture")} {
  auto hw_type = capture_.get_hardware_type();
  auto fw_vers = capture_.get_firmware_version();
  auto header_marker = capture_.read(ADDR_HEADER_MARKER);
  pflib_log(info) << "HW Type: " << hex(hw_type)
                  << " FW Version: " << hex(fw_vers)
                  << " Header Mark: " << hex(header_marker);
  // setting up with expected capture parameters
  capture_.write(ADDR_IDLE_PATTERN, 0x1277cc);
  // write header marker (0xAA followed by one bit)
  // location depends on firmware version
  if (capture_.get_firmware_version() < 0x10) {
    capture_.writeMasked(ADDR_HEADER_MARKER, MASK_HEADER_MARKER_OLD, 0x1E6);
  } else {
    capture_.writeMasked(ADDR_HEADER_MARKER, MASK_HEADER_MARKER, 0x1E6);
  }
  // zero should be 1
  if (capture_.readMasked(ADDR_PACKET_SETUP, MASK_L1A_PER_PACKET) == 0) {
    pflib_log(debug) << "Need to change MASK_L1A_PER_PACKET from 0 to 1";
    capture_.writeMasked(ADDR_PACKET_SETUP, MASK_L1A_PER_PACKET, 1);
  }
  // match to actual setup
  int samples_per_ror =
      capture_.readMasked(ADDR_PACKET_SETUP, MASK_L1A_PER_PACKET);
  int soi = capture_.readMasked(ADDR_PACKET_SETUP, MASK_SOI);
  pflib::DAQ::setup(samples_per_ror, soi);
  // TODO: handle multiple ECONs (needs support higher in the chain)
  capture_.writeMasked(ADDR_PICK_ECON, MASK_PICK_ECON, 0);
  for (int iecon=0; iecon < nlinks(); iecon++) {
    int econid = capture_.readMasked(ADDR_ECON0_ID + (iecon/2)*4, (iecon%2)?(MASK_ECON1_ID):(MASK_ECON0_ID));
    pflib::DAQ::setEconId(iecon, econid);
  }
}
void BWdaq::reset() {
  capture_.write(ADDR_EVB_CLEAR, MASK_EVB_CLEAR);  // auto-clear
}
int BWdaq::getEventOccupancy() {  // hmm... multiple econs...
  return getEventOccupancy(0);
}
int BWdaq::getEventOccupancy(int ilink) { 
  capture_.writeMasked(ADDR_PICK_ECON, MASK_PICK_ECON, ilink);
  auto nsamples = capture_.readMasked(ADDR_INFO, MASK_IO_NEVENTS);
  if (samples_per_ror() == 0) {
    pflib_log(warn) << "DAQ not configured, Samples/ROR set to zero.";
    return nsamples;
  }
  return nsamples / samples_per_ror();
}
void BWdaq::bufferStatus(int ilink, bool& empty, bool& full) {
  int nevt = getEventOccupancy(ilink);
  empty = (nevt == 0);
  full = (nevt == 0x7f);
}
void BWdaq::setup(int samples_per_ror, int soi) {
  pflib::DAQ::setup(samples_per_ror, soi);
  capture_.writeMasked(ADDR_PACKET_SETUP, MASK_L1A_PER_PACKET, samples_per_ror);
  capture_.writeMasked(ADDR_PACKET_SETUP, MASK_SOI, soi);
}
void BWdaq::setEconId(int iecon, int econid) {
  pflib::DAQ::setEconId(iecon, econid);
  capture_.writeMasked(
      ADDR_PICK_ECON, MASK_PICK_ECON,
      iecon);
  capture_.writeMasked(ADDR_ECON0_ID + (iecon/2)*4, (iecon%2)?(MASK_ECON1_ID):(MASK_ECON0_ID), econid);
}
void BWdaq::enable(bool doenable) {
  if (doenable)
    capture_.writeMasked(ADDR_ENABLE, MASK_ENABLE, 1);
  else
    capture_.writeMasked(ADDR_ENABLE, MASK_ENABLE, 0);
  pflib::DAQ::enable(doenable);
}
bool BWdaq::enabled() {
  return capture_.readMasked(ADDR_ENABLE, MASK_ENABLE);
}

void BWdaq::AXIS_enable(bool doenable) {
  if (doenable)
    capture_.writeMasked(ADDR_DISABLE_AXIS, MASK_DISABLE_AXIS, 0);
  else
    capture_.writeMasked(ADDR_DISABLE_AXIS, MASK_DISABLE_AXIS, 1);
}
bool BWdaq::AXIS_enabled() {
  return capture_.readMasked(ADDR_DISABLE_AXIS, MASK_DISABLE_AXIS) == 0;
}
std::vector<uint32_t> BWdaq::getLinkData(int ilink) {
  capture_.writeMasked(ADDR_PICK_ECON, MASK_PICK_ECON, ilink);
  uint32_t words = 0;
  std::vector<uint32_t> retval;
  static const uint32_t UBITS = 0x180;
  static const uint32_t LBITS = 0x07F;

  capture_.writeMasked(ADDR_PAGE_SPY, MASK_PAGE_SPY, 0);

  words = capture_.readMasked(ADDR_INFO, MASK_IO_SIZE_NEXT);

  uint32_t iold = 0xFFFFFF;
  for (uint32_t i = 0; i < words; i++) {
    if ((iold & UBITS) != (i & UBITS))  // new upper address block
      capture_.writeMasked(ADDR_PAGE_SPY, MASK_PAGE_SPY, (i >> 7));
    retval.push_back(capture_.read(ADDR_SPY_BASE + (i & LBITS) * 4));
    iold = i;
  }
  return retval;
}
void BWdaq::advanceLinkReadPtr(int ilink) {
  capture_.write(ADDR_ADV_IO, 1<<ilink);
}

std::map<std::string, uint32_t> BWdaq::get_debug(
    uint32_t ask) {
  std::map<std::string, uint32_t> dbg;
  using namespace pflib::utility;

  static const int stepsize = 4;
  /*
    FILE* f = fopen("dump.txt", "w");

    for (int i = 0; i < 0xFFF; i++)
    fprintf(f, "%03x %03x %08x\n", i, i, capture_.read(i));
    fclose(f);
  */

  dbg["AXIS.COUNT_WORDS"] = capture_.read(ADDR_BASE_COUNTER);
  dbg["AXIS.COUNT_TLAST"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 1);
  dbg["AXIS.COUNT_TVALID"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 2);
  dbg["AXIS.COUNT_TREADY"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 3);
  dbg["AXIS.COUNT_TVALID_!TREADY"] =
      capture_.read(ADDR_BASE_COUNTER + stepsize * 4);
  dbg["AXIS.COUNT_TREADY_!TVALID"] =
      capture_.read(ADDR_BASE_COUNTER + stepsize * 5);

  for (int ilink = 0; ilink < nlinks(); ilink++) {
    capture_.writeMasked(ADDR_PICK_ECON, MASK_PICK_ECON, ilink);
    dbg[string_format("ECON%d.COUNT_IDLES", ilink)] =
        capture_.read(ADDR_BASE_LINK_COUNTER);
    dbg[string_format("ECON%d.COUNT_NONIDLES", ilink)] =
        capture_.read(ADDR_BASE_LINK_COUNTER + stepsize * 1);
    dbg[string_format("ECON%d.COUNT_STARTS", ilink)] =
        capture_.read(ADDR_BASE_LINK_COUNTER + stepsize * 2);
    dbg[string_format("ECON%d.COUNT_STOPS", ilink)] =
        capture_.read(ADDR_BASE_LINK_COUNTER + stepsize * 3);
    dbg[string_format("ECON%d.COUNT_WORDS", ilink)] =
        capture_.read(ADDR_BASE_LINK_COUNTER + stepsize * 4);
    dbg[string_format("ECON%d.COUNT_IO_ADV", ilink)] =
        capture_.read(ADDR_BASE_LINK_COUNTER + stepsize * 5);
    dbg[string_format("ECON%d.COUNT_TLAST", ilink)] =
        capture_.read(ADDR_BASE_LINK_COUNTER + stepsize * 6);

    dbg[string_format("ECON%d.QUICKSPY", ilink)] =
        capture_.read(ADDR_BASE_LINK_COUNTER + 0x11 * stepsize);
    dbg[string_format("ECON%d.STATE", ilink)] =
        capture_.read(ADDR_BASE_LINK_COUNTER + 0x12 * stepsize);
    capture_.writeMasked(ADDR_PAGE_SPY, MASK_PAGE_SPY, 0);
    dbg[string_format("ECON%d.NEXT_EVENT_SIZE", ilink)] =
        capture_.readMasked(ADDR_INFO, MASK_IO_SIZE_NEXT);
  }
  return dbg;
}

}  // namespace bittware
}  // namespace pflib
