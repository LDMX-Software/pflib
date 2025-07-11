#include <math.h>

#include <fstream>
#include <iostream>

#include "pflib/menu/Menu.h"
#include "lpgbt_mezz_tester.h"
#include "zcu_optolink.h"
#include "pflib/lpgbt/lpGBT_ConfigTransport_I2C.h"
#include "pflib/lpgbt/lpGBT_Utility.h"

struct ToolBox {
  pflib::lpGBT* lpgbt;
  pflib::zcu::OptoLink* olink;
};

using tool = pflib::menu::Menu<ToolBox*>;

void opto(const std::string& cmd, ToolBox* target) {
  pflib::zcu::OptoLink& olink=*(target->olink);
  if (cmd=="FULLSTATUS") {
    std::map<std::string,uint32_t> info;
    info=olink.opto_status();
    printf("Optical status:");
    for (auto i:info) {
      printf("  %-20s : 0x%04x\n",i.first,i.second);
    }
    info=olink.opto_rates();
    printf("Optical rates:");
    for (auto i:info) {
      printf("  %-20s : 0x%04x\n",i.first,i.second);
    }
  }
}

void regs(const std::string& cmd, ToolBox* target) {
  static int addr = 0;
  if (cmd == "READ") {
    addr = tool::readline_int("Register", addr, true);
    int n = tool::readline_int("Number of items", 1);
    for (int i = 0; i < n; i++) {
      printf("   %03x : %02x\n", addr + i, target->lpgbt->read(addr + i));
    }
  }
  if (cmd == "WRITE") {
    addr = tool::readline_int("Register", addr, true);
    int value = target->lpgbt->read(addr);
    value = tool::readline_int("New value", value, true);
    target->lpgbt->write(addr, value);
  }
  if (cmd == "LOAD") {
    std::string fname = tool::readline("File: ");
    pflib::lpgbt::applylpGBTCSV(fname, *(target->lpgbt));
  }
}

void gpio(const std::string& cmd, ToolBox* target) {
  if (cmd == "SET") {
    int which = tool::readline_int("Pin");
    if (which >= 0 && which < 12) target->lpgbt->gpio_set(which, true);
  }
  if (cmd == "CLEAR") {
    int which = tool::readline_int("Pin");
    if (which >= 0 && which < 12) target->lpgbt->gpio_set(which, false);
  }
  if (cmd == "WRITE") {
    uint16_t value = target->lpgbt->gpio_get();
    value = tool::readline_int("Value", value, true);
    target->lpgbt->gpio_set(value);
  }
}

void adc(const std::string& cmd, ToolBox* target) {
  if (cmd == "READ") {
    int whichp = tool::readline_int("Channel (pos)");
    int whichn = tool::readline_int("Channel (neg)", 15);
    int gain = tool::readline_int("Gain", 1);
    printf("  ADC = %03x\n", target->lpgbt->adc_read(whichp, whichn, gain));
  }
  if (cmd == "ALL") {
    for (int whichp = 0; whichp < 15; whichp++) {
      printf("  ADC chan %d = %03x\n", whichp, target->lpgbt->adc_read(whichp, 15, 1));
    }
  }
}

static uint16_t lcl2gbt(uint16_t val) {
  uint16_t rval = 0;
  if (val & (1 << 0)) rval |= (1 << 9);
  if (val & (1 << 1)) rval |= (1 << 3);
  if (val & (1 << 2)) rval |= (1 << 6);
  if (val & (1 << 3)) rval |= (1 << 7);
  if (val & (1 << 4)) rval |= (1 << 1);
  if (val & (1 << 5)) rval |= (1 << 0);
  if (val & (1 << 6)) rval |= (1 << 2);
  if (val & (1 << 7)) rval |= (1 << 4);
  if (val & (1 << 8)) rval |= (1 << 10);
  if (val & (1 << 9)) rval |= (1 << 11);
  if (val & (1 << 10)) rval |= (1 << 8);
  if (val & (1 << 11)) rval |= (1 << 5);
  return rval;
}
static uint16_t gbt2lcl(uint16_t val) {
  uint16_t rval = 0;
  if (val & (1 << 0)) rval |= (1 << 5);
  if (val & (1 << 1)) rval |= (1 << 4);
  if (val & (1 << 2)) rval |= (1 << 6);
  if (val & (1 << 3)) rval |= (1 << 1);
  if (val & (1 << 4)) rval |= (1 << 7);
  if (val & (1 << 5)) rval |= (1 << 11);
  if (val & (1 << 6)) rval |= (1 << 2);
  if (val & (1 << 7)) rval |= (1 << 3);
  if (val & (1 << 8)) rval |= (1 << 10);
  if (val & (1 << 9)) rval |= (1 << 0);
  if (val & (1 << 10)) rval |= (1 << 8);
  if (val & (1 << 11)) rval |= (1 << 9);
  return rval;
}
#include "ad5593r.h"

void test(const std::string& cmd, ToolBox* target) {
  if (cmd == "GPIO") {
    // set up the external GPIO to read mode first
    pflib::lpGBT_ConfigTransport_I2C ext(0x21, "/dev/i2c-23");

    int errors = 0;

    int cycles = 10;
    printf("\n Testing..");
    fflush(stdout);
    do {
      ext.write_raw(0x8, 0xff);
      ext.write_raw(0x9, 0x0f);
      // setup the internal GPIO as write mode
      for (int i = 0; i < 12; i++) target->lpgbt->gpio_cfg_set(i, 1);  // output

      // walking ones lpgbt->lcl
      uint16_t pattern = 0x1;
      for (int i = 0; i < 12; i++) {
        target->lpgbt->gpio_set(pattern);
        ext.write_raw(0x80);
        uint16_t readback =
            (ext.read_raw() | (uint16_t(ext.read_raw()) << 8)) & 0xFFF;
        uint16_t readback2 = lcl2gbt(readback);
        if (readback2 != pattern) {
          printf("Set from lpGBT: %03x Read on I2C: %03x \n", pattern,
                 readback2);
          errors++;
        }
        pattern <<= 1;
      }
      // walking zeros lpgbt->lcl
      pattern = 0xFFE;
      for (int i = 0; i < 12; i++) {
        target->lpgbt->gpio_set(pattern);
        ext.write_raw(0x80);
        uint16_t readback =
            (ext.read_raw() | (uint16_t(ext.read_raw()) << 8)) & 0xFFF;
        uint16_t readback2 = lcl2gbt(readback);
        if (readback2 != pattern) {
          printf("Set from lpGBT: %03x Read on I2C: %03x \n", pattern,
                 readback2);
          errors++;
        }
        pattern <<= 1;
        pattern = (pattern & 0xFFF) | 1;
      }

      // switch directions
      // setup the internal GPIO as write mode
      for (int i = 0; i < 12; i++) target->lpgbt->gpio_cfg_set(i, 0);  // input
      ext.write_raw(0x8, 0x00);
      ext.write_raw(0x9, 0x00);

      // walking ones
      pattern = 0x1;
      for (int i = 0; i < 12; i++) {
        uint16_t scrambled = gbt2lcl(pattern);
        ext.write_raw(0xa, scrambled & 0xFF);
        ext.write_raw(0xb, scrambled >> 8);
        uint16_t readback = target->lpgbt->gpio_get();

        if (readback != pattern) {
          printf("Set from I2C: %03x Read on lpGBT: %03x \n", pattern,
                 readback);
          errors++;
        }
        pattern <<= 1;
      }
      // walking zeros
      pattern = 0xFFE;
      for (int i = 0; i < 12; i++) {
        uint16_t scrambled = gbt2lcl(pattern);
        ext.write_raw(0xa, scrambled & 0xFF);
        ext.write_raw(0xb, scrambled >> 8);
        uint16_t readback = target->lpgbt->gpio_get();

        if (readback != pattern) {
          printf("Set from I2C: %03x Read on lpGBT: %03x \n", pattern,
                 readback);
          errors++;
        }
        pattern <<= 1;
        pattern = (pattern & 0xFFF) | 1;
      }
      printf(".");
      fflush(stdout);
      cycles--;
    } while (cycles > 0 && errors == 0);
    if (errors == 0) printf("\n Test was OK\n");
  }
  if (cmd == "ADC") {
    pflib::AD5593R stim("/dev/i2c-23", 0x10);
    for (int i = 0; i < 4; i++) stim.setup_dac(i);

    uint16_t onevolt = uint16_t(0xfff / 2.5 * 1.0);

    // get the pedestal, DACs set to zero at this point
    int pedestal = target->lpgbt->adc_read(0, 15, 1);

    if (pedestal < 10 || pedestal > 50) {
      printf("Pedestal of lpGBT ADC (%d) is out of acceptable range", pedestal);
      return;
    }

    stim.dac_write(0, onevolt);
    stim.dac_write(1, onevolt);
    int fullrange = target->lpgbt->adc_read(0, 15, 1);

    if (fullrange < 0x3D0 || fullrange > 0x3FD) {
      printf("One volt range of lpGBT ADC (%d/0x%x) is out of acceptable range",
             fullrange, fullrange);
      return;
    }

    double scale = 1.0 / (fullrange - pedestal);
    int errors = 0;

    // test matrix
    for (int half = 0; half < 2; half++) {
      for (int pta = 0; pta < 3; pta++) {
        for (int ptb = 0; ptb < 3; ptb++) {
          stim.dac_write(0 + 2 * half, onevolt / 2 * pta);
          stim.dac_write(1 + 2 * half, onevolt / 2 * ptb);
          double Va = pta * 1.0 / 2;
          double Vb = ptb * 1.0 / 2;
          //	  printf("Setting Half %d %d %d\n",half,pta,ptb);
          /// resistor chain pta -> 200 -> ch 0 -> 100 -> ch 1 -> 100 -> ch 2 -> 100 -> ch 3 -> 200 -> ptb
          double curr = (Vb - Va) / (200 * 2 + 100 * 3);
          //	  for (int i=0+4*half; i<4+4*half; i++) {
          for (int i = 0 + 4 * half; i < 4 + 4 * half; i++) {
            double expected = Va + curr * 200 + curr * 100 * (i - 4 * half);
            if (half && i == 4) {  // resistor swap on test board
              expected = Va + curr * 100 + curr * 100 * (i - 4 * half);
            }
            int adc = target->lpgbt->adc_read(i, 15, 1);
            double volts = (adc - pedestal) * scale;
            double error = expected - volts;
            // due to resistor chain, compliance isn't perfect when current flow is non-trivial.  Allowable error scales with deltaV as a result
            double error_ok = 5e-3 + (15e-3 * abs(pta - ptb));
            if (fabs(error) > error_ok) {
              errors++;
              printf(" ADC%d %d %f %f %f %f\n", i, adc, volts, expected, error,
                     error_ok);
            }
          }
        }
      }
    }
    if (errors == 0)
      printf(" ADC TEST PASS\n");
    else
      printf(" ADC TEST ERRORS : %d\n", errors);
    for (int i = 0; i < 4; i++) stim.clear_pin(i);
  }
  if (cmd == "ECLK") {
    LPGBT_Mezz_Tester mezz;
    int errors = 0;
    for (int iclk = 0; iclk < 8; iclk++) {
      static constexpr int BIN[4] = {40, 80, 160, 320};
      for (int ibin = 0; ibin < 4; ibin++) {
        target->lpgbt->setup_eclk(iclk, BIN[ibin]);
        usleep(100000);
        std::vector<float> rates = mezz.clock_rates();
        float measured = rates[iclk];
        if (fabs(measured - BIN[ibin]) > 5) {
          errors++;
          printf("%d set %d measured %f\n", iclk, BIN[ibin], rates[iclk]);
        }
        target->lpgbt->setup_eclk(iclk, 0);
      }
    }
    if (errors != 0)
      printf("  ECLK TEST FAILED\n");
    else
      printf("  ECLK TEST PASS\n");
  }
}

namespace {

auto optom = tool::menu("OPTO", "Optical Link Functions")
    ->line("FULLSTATUS", "Get full status", opto);

auto direct = tool::menu("REG", "Direct Register Actions")
                  ->line("READ", "Read one or several registers", regs)
                  ->line("WRITE", "Write a register", regs)
                  ->line("LOAD", "Load from a CSV file", regs);

auto mgpio = tool::menu("GPIO", "GPIO controls")
                 ->line("SET", "Set a GPIO pin", gpio)
                 ->line("CLEAR", "CLEAR a GPIO pin", gpio)
                 ->line("WRITE", "Write all GPIO pins at once", gpio);

auto madc = tool::menu("ADC", "ADC and DAC-related actions")
                ->line("READ", "Read an ADC line", adc)
                ->line("ALL", "Read all ADC lines", adc);

auto mtest = tool::menu("TEST", "Mezzanine testing functions")
                 ->line("GPIO", "Test the gpio functions", test)
                 ->line("ADC", "Test the ADC function", test)
                 ->line("ECLK", "Test the elocks", test);

}  // namespace

int main(int argc, char* argv[]) {
  if (argc == 2 and (!strcmp(argv[1], "-h") or !strcmp(argv[1], "--help"))) {
    printf("\nUSAGE: \n");
    printf("   %s -z OPTIONS\n\n", argv[0]);
    printf("OPTIONS:\n");
    printf("  -s [file] : pass a script of commands to run through\n");
    printf("  -h|--help : print this help and exit\n");
    printf("\n");
    return 1;
  }

  for (int i = 1; i < argc; i++) {
    std::string arg(argv[i]);
    if (arg == "-s") {
      if (i + 1 == argc or argv[i + 1][0] == '-') {
        std::cerr << "Argument " << arg << " requires a file after it."
                  << std::endl;
        return 2;
      }
      i++;
      std::fstream sFile(argv[i]);
      if (!sFile.is_open()) {
        std::cerr << "Unable to open script file " << argv[i] << std::endl;
        return 2;
      }
      std::string line;
      while (getline(sFile, line)) {
        // erase whitespace at beginning of line
        while (!line.empty() && isspace(line[0])) line.erase(line.begin());
        // skip empty lines or ones whose first character is #
        if (!line.empty() && line[0] == '#') continue;
        // add to command queue
        tool::add_to_command_queue(line);
      }
      sFile.close();
    }
  }

  pflib::lpGBT_ConfigTransport_I2C tport(0x79, "/dev/i2c-23");
  pflib::lpGBT lpgbt(tport);
  pflib::zcu::OptoLink olink;
  tool::set_history_filepath("~/.pflpgbt-history");
  ToolBox t;
  t.lpgbt=&lpgbt;
  t.olink=&olink;  
  tool::run(&t);

  return 0;
}
