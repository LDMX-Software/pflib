#include "lpgbt_mezz_tester.h"
#include <boost/process.hpp>


LPGBT_Mezz_Tester::LPGBT_Mezz_Tester() : uio_("lpgbtmezz_test") {
  //  printf("%08x %08x\n", uio_.read(0x20), uio_.read(0x21));
}

std::vector<float> LPGBT_Mezz_Tester::clock_rates() {
  std::vector<float> retvals;
  for (int iclk = 0; iclk < 8; iclk++) {
    uint32_t val = uio_.read(0x20 + 8 + iclk);
    retvals.push_back(val / 1e4);
  }
  return retvals;
}

static const char* GPIO_STRING="4";

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
