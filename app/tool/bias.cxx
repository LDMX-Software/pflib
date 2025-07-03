/**
 * @file bias.cxx
 *
 * Definition of BIAS menu commands
 */
#include "pftool.h"

ENABLE_LOGGING();

static void bias( const std::string& cmd, Target* pft ) {
  static int iboard=0;
  if (cmd=="READ_SIPM"){
    iboard=pftool::readline_int("Which board? ",iboard);
    pflib::Bias bias=pft->hcal().bias(iboard);
    int ich=pftool::readline_int("Which (zero-indexed) channel? (-1 for all) ",iboard);
    if(ich == -1){
      for(int i = 0; i < 16; i++){
        std::cout << "Channel " << i << ": " << bias.readSiPM(i) << std::endl;
      }
    } else {
      std::cout << "Channel " << ich << ": " << bias.readSiPM(ich) << std::endl;
    }
  }
  if (cmd=="READ_LED"){
    iboard=pftool::readline_int("Which board? ",iboard);
    pflib::Bias bias=pft->hcal().bias(iboard);
    int ich=pftool::readline_int("Which (zero-indexed) channel? (-1 for all) ",iboard);
    if(ich == -1){
      for(int i = 0; i < 16; i++){
        std::cout << "Channel " << i << ": " << bias.readLED(i) << std::endl;
      }
    } else {
      std::cout << "Channel " << ich << ": " << bias.readLED(ich) << std::endl;
    }
  }
  if (cmd=="SET_SIPM"){
    iboard=pftool::readline_int("Which board? ",iboard);
    pflib::Bias bias=pft->hcal().bias(iboard);
    int ich=pftool::readline_int("Which (zero-indexed) channel? (-1 for all) ",iboard);
    uint16_t dac=pftool::readline_int("Which DAC value? ", 0);
    if(ich == -1){
      for(int i = 0; i < 16; i++){
        bias.setSiPM(i, dac);
      }
    } else {
      bias.setSiPM(ich, dac);
    }
  }
  if (cmd=="SET_LED"){
    iboard=pftool::readline_int("Which board? ",iboard);
    pflib::Bias bias=pft->hcal().bias(iboard);
    int ich=pftool::readline_int("Which (zero-indexed) channel? (-1 for all) ",iboard);
    uint16_t dac=pftool::readline_int("Which DAC value? ", 0);
    if(ich == -1){
      for(int i = 0; i < 16; i++){
        bias.setLED(i, dac);
      }
    } else {
      bias.setLED(ich, dac);
    }
  }
}

namespace {
auto menu_bias =
    pftool::menu("BIAS", "Read and write bias voltages")
    ->line("READ_SIPM","Read SiPM DAC values", bias )
    ->line("READ_LED","Read LED DAC values", bias )
    ->line("SET_SIPM","Set SiPM DAC values", bias )
    ->line("SET_LED","Set LED DAC values", bias )
;
}
