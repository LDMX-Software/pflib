/**
 * @file bias.cxx
 *
 * Definition of BIAS menu commands
 *
 * Only usable for HcalBackplane type targets.
 */
#include "pflib/HcalTarget.h"
#include "pflib/Bias.h"
#include "pftool.h"

ENABLE_LOGGING();

std::ostream& operator<<(std::ostream& o, std::optional<int> val) {
  if (val) {
    o << std::setw(4) << val.value();
  } else {
    o << "????";
  }
  return o;
}

static void bias(const std::string& cmd, pflib::HcalTarget* pft) {
  static int iboard = 0;
  if (cmd == "STATUS") {
    iboard = pftool::readline_int("Which board? ", iboard);
    pflib::Bias& bias = pft->bias(iboard);
    double temp = bias.readTemp();
    std::cout << "Board temperature: " << temp << " C" << std::endl;
    for (int ch = 0; ch < 16; ch++) {
      std::cout << "Channel " << ch << " SiPM DAC " << bias.readSiPM(ch)
                << " LED DAC " << bias.readLED(ch) << std::endl;
    }
  }
  if (cmd == "READ_SIPM") {
    iboard = pftool::readline_int("Which board? ", iboard);
    pflib::Bias& bias = pft->bias(iboard);
    int ich = pftool::readline_int(
        "Which (zero-indexed) channel? (-1 for all) ", iboard);
    if (ich == -1) {
      for (int i = 0; i < 16; i++) {
        std::cout << "Channel " << std::setw(2) << i << ": " << bias.readSiPM(i) << std::endl;
      }
    } else {
      std::cout << "Channel " << std::setw(2) << ich << ": " << bias.readSiPM(ich) << std::endl;
    }
  }
  if (cmd == "READ_LED") {
    iboard = pftool::readline_int("Which board? ", iboard);
    pflib::Bias& bias = pft->bias(iboard);
    int ich = pftool::readline_int(
        "Which (zero-indexed) channel? (-1 for all) ", iboard);
    if (ich == -1) {
      for (int i = 0; i < 16; i++) {
        std::cout << "Channel " << std::setw(2) << i << ": " << bias.readLED(i) << std::endl;
      }
    } else {
      std::cout << "Channel " << std::setw(2) << ich << ": " << bias.readLED(ich) << std::endl;
    }
  }
  if (cmd == "SET_SIPM") {
    iboard = pftool::readline_int("Which board? ", iboard);
    pflib::Bias& bias = pft->bias(iboard);
    int ich = pftool::readline_int(
        "Which (zero-indexed) channel? (-1 for all) ", iboard);
    uint16_t dac = pftool::readline_int("Which DAC value? ", 0);
    if (ich == -1) {
      for (int i = 0; i < 16; i++) {
        bias.setSiPM(i, dac);
      }
    } else {
      bias.setSiPM(ich, dac);
    }
  }
  if (cmd == "SET_LED") {
    iboard = pftool::readline_int("Which board? ", iboard);
    pflib::Bias& bias = pft->bias(iboard);
    int ich = pftool::readline_int(
        "Which (zero-indexed) channel? (-1 for all) ", iboard);
    uint16_t dac = pftool::readline_int("Which DAC value? ", 0);
    if (ich == -1) {
      for (int i = 0; i < 16; i++) {
        bias.setLED(i, dac);
      }
    } else {
      bias.setLED(ich, dac);
    }
  }
  if (cmd == "INIT") {
    iboard = pftool::readline_int("Which board? ", iboard);
    pft->bias(iboard).initialize();
  }
  if (cmd == "READ_TEMP") {
    iboard = pftool::readline_int("Which board? ", iboard);
    double temp = pft->bias(iboard).readTemp();
    std::cout << "Board temperature: " << temp << " C" << std::endl;
  }
}

static void render(Target* tgt) {
  auto hcal = dynamic_cast<pflib::HcalTarget*>(tgt);
  if (not hcal) {
    pflib_log(error)
        << "BIAS menu of commands only availabe for Hcal targets.";
  }
  if (pftool::state.readout_config() != pftool::State::CFG_HCALFMC) {
    std::cout << R"WARN(
  We currently do not have the ability to readback the voltage settings
  from the MAX5825 via the lpGBT. This means we cache the last value we
  wrote to the board in the software.
  A value of '????' means we haven't written to that channel of the board,
  so we do not know what value that channel is set to.
  In this case, either set the channel to the desired setting or check the
  board manually with a multimeter.
)WARN" << std::endl;
  }
}

static void bias_wrapper(const std::string& cmd, Target* tgt) {
  auto hcal = dynamic_cast<pflib::HcalTarget*>(tgt);
  if (hcal) {
    bias(cmd, hcal);
  } else {
    PFEXCEPTION_RAISE("NotImpl",
                      "The BIAS menu of commands is only available for "
                      "Hcal targets.");
  }
}

namespace {
auto menu_bias =
    pftool::menu("BIAS", "Read and write bias voltages", render, ONLY_HCAL)
        ->line("STATUS", "Bias and board I2C summary", bias_wrapper)
        ->line("INIT", "Board I2C Initialization", bias_wrapper)
        ->line("READ_SIPM", "Read SiPM DAC values", bias_wrapper)
        ->line("READ_LED", "Read LED DAC values", bias_wrapper)
        ->line("SET_SIPM", "Set SiPM DAC values", bias_wrapper)
        ->line("SET_LED", "Set LED DAC values", bias_wrapper)
        ->line("READ_TEMP", "Read temperature", bias_wrapper);
}
