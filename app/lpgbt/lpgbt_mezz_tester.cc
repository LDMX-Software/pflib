#include "lpgbt_mezz_tester.h"
#include <boost/process.hpp>

LPGBT_Mezz_Tester::LPGBT_Mezz_Tester(pflib::UIO& opto) : opto_{opto}, wired_("lpgbtmezz_test") {
  //  printf("%08x %08x\n", wired_.read(0x20), wired_.read(0x21));
}

std::vector<float> LPGBT_Mezz_Tester::clock_rates() {
  std::vector<float> retvals;
  for (int iclk = 0; iclk < 8; iclk++) {
    uint32_t val = wired_.read(0x20 + 8 + iclk);
    retvals.push_back(val / 1e4);
  }
  return retvals;
}

static const char* GPIO_STRING="4";

static const uint32_t REG_PRBS_LEN          = 1;
static const uint32_t REG_CTL               = 0;
static const uint32_t REG_WIRED_DELAY_CAPTURE_WHICH = 2;
static const uint32_t REG_UPLINK_PATTERN    = 5;
static const uint32_t REG_PRBS_BUSY         = 0x22;
static const uint32_t REG_WIRED_CAPTURE     = 0x100;
static const uint32_t REG_WIRED_ERROR_COUNT = 0x160;
static const uint32_t MASK_PRBS_CLEAR       = 0x00000004;
static const uint32_t MASK_PRBS_RESET       = 0x01000000;
static const uint32_t MASK_START_PRBS       = 0x00010000;
static const uint32_t MASK_ENABLE_CAPTURE   = 0x00020000;
static const uint32_t MASK_WIRED_TX_DELAY      = 0x01FF0000;
static const uint32_t MASK_WIRED_RX_DELAY      = 0x0000FF80;
static const uint32_t MASK_WIRED_CAPTURE_WHICH = 0x0000000F;

void LPGBT_Mezz_Tester::get_mode(bool& addr, bool& mode) {
  boost::process::ipstream is;

  boost::process::child c(boost::process::search_path("gpioget"),GPIO_STRING,"0", "1", "2", "3", "4","5",boost::process::std_out > is);

  std::string line;
  bool done=false;
  int values[6];
  while (c.running() && std::getline(is,line) && !line.empty()) {
    if (!done) {
      if (sscanf(line.c_str(),"%d %d %d %d %d %d",&(values[0]),&(values[1]),&(values[2]),&(values[3]),&(values[4]),&(values[5]))==6) done=true;
    }
  }
  c.wait();
  if (done) {
    printf("%d %d %d %d %d %d\n",(values[0]),(values[1]),(values[2]),(values[3]),(values[4]),(values[5]));
    addr=values[0];
    mode=values[1];
  }
  
}

void LPGBT_Mezz_Tester::set_mode(bool addr, bool mode) {
  char cmd[100];
  snprintf(cmd,100,"gpioset 4 0=%d 1=%d",(addr)?(1):(0),(mode)?(1):(0));
  boost::process::system((const char*)(cmd), boost::process::std_out > boost::process::null);
}

void LPGBT_Mezz_Tester::reset_lpGBT() {
  boost::process::system("4 2=0", boost::process::std_out > boost::process::null);
  boost::process::system("4 2=1", boost::process::std_out > boost::process::null);
}

void LPGBT_Mezz_Tester::set_prbs_len_ms(int len_ms) {
  wired_.write(REG_PRBS_LEN,len_ms);
  static constexpr int REG_OPTO_PRBS_LEN = 98;
  opto_.write(REG_OPTO_PRBS_LEN,len_ms);
}

void LPGBT_Mezz_Tester::set_phase(int phase) {
  wired_.rmw(REG_WIRED_DELAY_CAPTURE_WHICH, MASK_WIRED_TX_DELAY, phase<<16);
  wired_.rmw(REG_WIRED_DELAY_CAPTURE_WHICH, MASK_WIRED_RX_DELAY, phase<<7);
}

void LPGBT_Mezz_Tester::set_uplink_pattern(int ilink, int pattern) {
  if (ilink<0 || ilink>=7) return;
  uint32_t val=wired_.read(REG_UPLINK_PATTERN);
  uint32_t mask=(0xF)<<(ilink*4);
  val=val|mask;
  val=val^mask;
  val=val|((pattern&0xF)<<(ilink*4));
  wired_.write(REG_UPLINK_PATTERN,val);
}

std::vector<uint32_t> LPGBT_Mezz_Tester::ber_tx() {
  // clear the counters
  wired_.rmw(REG_CTL,MASK_PRBS_CLEAR,MASK_PRBS_CLEAR);
  usleep(1);
  wired_.rmw(REG_CTL,MASK_PRBS_CLEAR,0);
  // start the PRBS
  wired_.rmw(REG_CTL,MASK_START_PRBS,MASK_START_PRBS);
  bool busy=true;
  do {
    usleep(100); // takes some time...
    busy=(wired_.read(REG_PRBS_BUSY)&0x1)!=0;
  } while (busy);
  std::vector<uint32_t> retval;
  for (int i=0; i<7; i++) {
    retval.push_back(wired_.read(REG_WIRED_ERROR_COUNT+i));
  }
  return retval;
}

std::vector<uint32_t> LPGBT_Mezz_Tester::ber_rx() {
  static constexpr int REG_UPLINK_PRBS_RESET = 96;
  static constexpr int REG_UPLINK_PRBS_START = 97;
  static constexpr int REG_UPLINK_PRBS_BUSY  = 100;
  static constexpr int REG_UPLINK_PRBS_ERROR_COUNT = 101;
  
  // clear the counters
  opto_.rmw(REG_UPLINK_PRBS_RESET,1,1);
  usleep(1);
  // start the PRBS
  opto_.rmw(REG_UPLINK_PRBS_START,1,1);
  bool busy=true;
  do {
    usleep(100); // takes some time...
    busy=(opto_.read(REG_UPLINK_PRBS_BUSY)&0x1)!=0;
  } while (busy);
  std::vector<uint32_t> retval;
  for (int i=0; i<6; i++) {
    int reg=REG_UPLINK_PRBS_ERROR_COUNT+i;
    if (i>=3) reg++;
    retval.push_back(opto_.read(reg));
  }
  return retval;
}

std::vector<uint32_t> LPGBT_Mezz_Tester::capture(int ilink, bool is_rx) {
  std::vector<uint32_t> retval;
  if (!is_rx) {
    wired_.rmw(REG_WIRED_DELAY_CAPTURE_WHICH, MASK_WIRED_CAPTURE_WHICH, ilink&0xFF);
    wired_.rmw(REG_CTL, MASK_ENABLE_CAPTURE, MASK_ENABLE_CAPTURE); // self-clearing
    usleep(1000); // wait...
    for (int i=0; i<32; i++) {
      retval.push_back(wired_.read(REG_WIRED_CAPTURE+i));
    }
  } else { // high-speed rx
    static constexpr int REG_CAPTURE_ENABLE = 16;
    static constexpr int REG_CAPTURE_ELINK  = 18;
    static constexpr int REG_CAPTURE_OLINK  = 17;
    static constexpr int REG_CAPTURE_PTR    = 19;
    static constexpr int REG_CAPTURE_WINDOW = 20;
    opto_.write(REG_CAPTURE_OLINK,0);
    opto_.write(REG_CAPTURE_ELINK,ilink&0x7);
    opto_.write(REG_CAPTURE_ENABLE,1);
    usleep(1000);
    opto_.write(REG_CAPTURE_ENABLE,0);
    for (int i=0; i<32; i++) {
      opto_.write(REG_CAPTURE_PTR,i);
      usleep(1);
      retval.push_back(opto_.read(REG_CAPTURE_WINDOW));
    }    
  }
  return retval;
}


void LPGBT_Mezz_Tester::capture_ec(int mode, std::vector<uint8_t>& tx, std::vector<uint8_t>& rx) {
  static constexpr int REG_SPY_CTL = 67;
  static constexpr int REG_READ    = 69;

  uint32_t val=opto_.read(REG_SPY_CTL);
  val=val&0x1FF;
  val=val|((mode&0x3)<<10)|(1<<9); 
  opto_.write(REG_SPY_CTL,val);

  // now wait for it...
  bool done=false;
  do {
    usleep(100);
    done=(opto_.read(REG_READ)&0x400)!=0;
    if (mode==0) done=true;
  } while (!done);
  val=val&0x1FF;
  val=val|((mode&0x3)<<10); // disable the spy 
  opto_.write(REG_SPY_CTL,val);

  tx.clear();
  rx.clear();
  
  val=val&0x1FF;
  val=val|((mode&0x3)<<10); 
  for (int i=0; i<256; i++) {
    val=val&0xFFF;
    val=val|(i<<12);
    opto_.write(REG_SPY_CTL,val);
    usleep(1);
    uint32_t k=opto_.read(REG_READ);
    tx.push_back((k>>(2+11))&0x3);
    rx.push_back((k>>11)&0x3);
  }
  
}
