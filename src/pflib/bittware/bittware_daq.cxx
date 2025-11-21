#include "pflib/bittware/bittware_daq.h"
#include "pflib/utility/string_format.h"

namespace pflib {
namespace bittware {

static constexpr uint32_t BASE_ADDRESS_CAPTURE0 = 0x8000;

static constexpr uint32_t ADDR_IDLE_PATTERN = 0x604;
static constexpr uint32_t ADDR_HEADER_MARKER = 0x600;
static constexpr uint32_t MASK_HEADER_MARKER = 0x0001FF00;
static constexpr uint32_t ADDR_ENABLE = 0x600;
static constexpr uint32_t MASK_ENABLE = 0x00000001;
static constexpr uint32_t ADDR_EVB_CLEAR = 0x100;
static constexpr uint32_t MASK_EVB_CLEAR = 0x00000001;
static constexpr uint32_t ADDR_ADV_IO = 0x080;
// static constexpr uint32_t MASK_ADV_IO = 0x00000001;

static constexpr uint32_t ADDR_PACKET_SETUP   = 0x400;
static constexpr uint32_t MASK_L1A_PER_PACKET = 0x000001F0;
static constexpr uint32_t MASK_SOI            = 0x00001E00;
static constexpr uint32_t ADDR_PICK_ECON      = 0x400;
static constexpr uint32_t MASK_PICK_ECON      = 0x000F0000;

static constexpr uint32_t ADDR_ECON0_ID     = 0x408; //
static constexpr uint32_t MASK_ECON0_ID     = 0x03FF;

static constexpr uint32_t ADDR_PAGE_SPY = 0x400;
static constexpr uint32_t MASK_PAGE_SPY = 0x00700000;

static constexpr uint32_t ADDR_SPY_BASE = 0xA00; // 0xA00 -> 0xBFC

static constexpr uint32_t ADDR_BASE_COUNTER = 0x880;   
static constexpr uint32_t ADDR_INFO = 0x8C0;   

static constexpr uint32_t MASK_IO_NEVENTS = 0x0000007F;
static constexpr uint32_t MASK_IO_SIZE_NEXT = 0x0000FF80;

HcalBackplaneBW_Capture::HcalBackplaneBW_Capture(const char* dev) : DAQ(1), capture_(BASE_ADDRESS_CAPTURE0, dev) {
  printf("Firmware type and version: %08x %08x %08x\n",capture_.read(0),capture_.read(ADDR_IDLE_PATTERN),capture_.read(ADDR_HEADER_MARKER));
  // setting up with expected capture parameters
  capture_.write(ADDR_IDLE_PATTERN, 0x1277cc);
  capture_.writeMasked(ADDR_HEADER_MARKER, MASK_HEADER_MARKER,
                       0x1E6);  // 0xAA followed by one bit...
}
void HcalBackplaneBW_Capture::reset() {
  capture_.write(ADDR_EVB_CLEAR, MASK_EVB_CLEAR);  // auto-clear
}
int HcalBackplaneBW_Capture::getEventOccupancy() { // hmm... multiple econs...
  capture_.writeMasked(ADDR_PICK_ECON,MASK_PICK_ECON,0);
  return capture_.readMasked(ADDR_INFO, MASK_IO_NEVENTS) /
      samples_per_ror();
}
void setupLink(int ilink, int l1a_delay, int l1a_capture_width) {
  // none of these parameters are relevant for the econd capture, which is
  // data-pattern based
}
void getLinkSetup(int ilink, int& l1a_delay, int& l1a_capture_width) {
  l1a_delay = -1;
  l1a_capture_width = -1;
}
void HcalBackplaneBW_Capture::bufferStatus(int ilink, bool& empty, bool& full) {
  int nevt = getEventOccupancy();
  empty = (nevt == 0);
  full = (nevt == 0x7f);
}
void HcalBackplaneBW_Capture::setup(int econid, int samples_per_ror, int soi) {
  pflib::DAQ::setup(econid, samples_per_ror, soi);
  capture_.writeMasked(ADDR_PACKET_SETUP, MASK_L1A_PER_PACKET,
                       samples_per_ror);
  capture_.writeMasked(ADDR_PACKET_SETUP, MASK_SOI, soi);
  capture_.writeMasked(ADDR_PICK_ECON,MASK_PICK_ECON,0); // TODO: handle multiple ECONs (needs support higher in the chain)
  capture_.writeMasked(ADDR_ECON0_ID, MASK_ECON0_ID, econid);
}
void HcalBackplaneBW_Capture::enable(bool doenable) {
  if (doenable)
    capture_.writeMasked(ADDR_ENABLE, MASK_ENABLE, 1);
  else
    capture_.writeMasked(ADDR_ENABLE, MASK_ENABLE, 0);
  pflib::DAQ::enable(doenable);
}
bool HcalBackplaneBW_Capture::enabled() {
  return capture_.readMasked(ADDR_ENABLE, MASK_ENABLE);
}
std::vector<uint32_t> HcalBackplaneBW_Capture::getLinkData(int ilink) {
  capture_.writeMasked(ADDR_PICK_ECON,MASK_PICK_ECON,ilink);
  uint32_t words = 0;
  std::vector<uint32_t> retval;
  static const uint32_t UBITS = 0x180;
  static const uint32_t LBITS = 0x07F;
  
  capture_.writeMasked(ADDR_PAGE_SPY, MASK_PAGE_SPY,
                       0); 
  
  words = capture_.readMasked(ADDR_INFO, MASK_IO_SIZE_NEXT);
  
  uint32_t iold = 0xFFFFFF;
  for (uint32_t i = 0; i < words; i++) {
    if ((iold & UBITS) != (i & UBITS))  // new upper address block 
      capture_.writeMasked(ADDR_PAGE_SPY, MASK_PAGE_SPY,
                           (i >> 7));
    retval.push_back(capture_.read(ADDR_SPY_BASE + (i & LBITS)*4));
    iold=i;
  }
  return retval;
}
void HcalBackplaneBW_Capture::advanceLinkReadPtr() {
  capture_.write(ADDR_ADV_IO, 1);  // auto-clear, only correct for one econ right now
}

std::map<std::string, uint32_t> HcalBackplaneBW_Capture::get_debug(uint32_t ask) {
  std::map<std::string, uint32_t> dbg;
  static const int stepsize = 4;
  /*
    FILE* f = fopen("dump.txt", "w");
    
    for (int i = 0; i < 0xFFF; i++)
    fprintf(f, "%03x %03x %08x\n", i, i, capture_.read(i));
    fclose(f);
  */
  dbg["COUNT_IDLES"] = capture_.read(ADDR_BASE_COUNTER);
  dbg["COUNT_NONIDLES"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 1);
  dbg["COUNT_STARTS"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 2);
  dbg["COUNT_STOPS"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 3);
  dbg["COUNT_WORDS"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 4);
  dbg["COUNT_IO_ADV"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 5);
  dbg["COUNT_TLAST"] = capture_.read(ADDR_BASE_COUNTER + stepsize * 6);
  /*
    dbg["QUICKSPY"] = capture_.read(ADDR_BASE_COUNTER + 0x10);
    dbg["STATE"] = capture_.read(ADDR_BASE_COUNTER + 0x11);
  */
  return dbg;
}


}
}  // namespace pflib
