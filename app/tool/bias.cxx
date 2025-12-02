/**
 * @file bias.cxx
 *
 * Definition of BIAS menu commands
 *
 * Only usable for HcalBackplane type targets.
 */
#include "pflib/HcalBackplane.h"
#include "pftool.h"

ENABLE_LOGGING();

static void bias(const std::string& cmd, pflib::HcalBackplane* pft) {
  static int iboard = 0;
  if (cmd == "STATUS") {
    iboard = pftool::readline_int("Which board? ", iboard);
    pflib::Bias bias = pft->bias(iboard);
    double temp = bias.readTemp();
    std::cout << "Board temperature: " << temp << " C" << std::endl;
    for (int ch = 0; ch < 16; ch++) {
      std::cout << "Channel " << ch << " SiPM DAC " << bias.readSiPM(ch)
                << " LED DAC " << bias.readLED(ch) << std::endl;
    }
  }
  if (cmd == "READ_SIPM") {
    iboard = pftool::readline_int("Which board? ", iboard);
    pflib::Bias bias = pft->bias(iboard);
    int ich = pftool::readline_int(
        "Which (zero-indexed) channel? (-1 for all) ", iboard);
    if (ich == -1) {
      for (int i = 0; i < 16; i++) {
        std::cout << "Channel " << i << ": " << bias.readSiPM(i) << std::endl;
      }
    } else {
      std::cout << "Channel " << ich << ": " << bias.readSiPM(ich) << std::endl;
    }
  }
  if (cmd == "READ_LED") {
    iboard = pftool::readline_int("Which board? ", iboard);
    pflib::Bias bias = pft->bias(iboard);
    int ich = pftool::readline_int(
        "Which (zero-indexed) channel? (-1 for all) ", iboard);
    if (ich == -1) {
      for (int i = 0; i < 16; i++) {
        std::cout << "Channel " << i << ": " << bias.readLED(i) << std::endl;
      }
    } else {
      std::cout << "Channel " << ich << ": " << bias.readLED(ich) << std::endl;
    }
  }
  if (cmd == "SET_SIPM") {
    iboard = pftool::readline_int("Which board? ", iboard);
    pflib::Bias bias = pft->bias(iboard);
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
    pflib::Bias bias = pft->bias(iboard);
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
    pflib::Bias bias = pft->bias(iboard);
    bias.initialize();
  }
  if (cmd == "READ_TEMP") {
    iboard = pftool::readline_int("Which board? ", iboard);
    pflib::Bias bias = pft->bias(iboard);
    double temp = bias.readTemp();
    std::cout << "Board temperature: " << temp << " C" << std::endl;
  }
}

static void render(Target* tgt) {
  auto hcal = dynamic_cast<pflib::HcalBackplane*>(tgt);
  if (not hcal) {
    pflib_log(error)
        << "BIAS menu of commands only availabe for HcalBackplane targets.";
  }
}

static void bias_wrapper(const std::string& cmd, Target* tgt) {
  auto hcal = dynamic_cast<pflib::HcalBackplane*>(tgt);
  if (hcal) {
    bias(cmd, hcal);
  } else {
    PFEXCEPTION_RAISE("NotImpl",
                      "The BIAS menu of commands is only available for "
                      "HcalBackplane targets.");
  }
}

namespace {
auto menu_bias =
    pftool::menu("BIAS", "Read and write bias voltages", render)
        ->line("STATUS", "Bias and board I2C summary", bias_wrapper)
        ->line("INIT", "Board I2C Initialization", bias_wrapper)
        ->line("READ_SIPM", "Read SiPM DAC values", bias_wrapper)
        ->line("READ_LED", "Read LED DAC values", bias_wrapper)
        ->line("SET_SIPM", "Set SiPM DAC values", bias_wrapper)
        ->line("SET_LED", "Set LED DAC values", bias_wrapper)
        ->line("READ_TEMP", "Read temperature", bias_wrapper);
}
