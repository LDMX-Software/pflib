#include <math.h>

#include <fstream>
#include <iostream>

#include "lpgbt_mezz_tester.h"
#include "pflib/lpgbt/lpGBT_ConfigTransport_I2C.h"
#include "pflib/lpgbt/lpGBT_Utility.h"
#include "pflib/lpgbt/lpGBT_standard_configs.h"
#include "pflib/menu/Menu.h"
#include "pflib/zcu/lpGBT_ICEC_ZCU_Simple.h"
#include "pflib/zcu/zcu_optolink.h"
#include "power_ctl_mezz.h"

struct ToolBox {
  pflib::lpGBT* lpgbt{0};
  pflib::lpGBT* lpgbt_i2c{0};
  pflib::lpGBT* lpgbt_ic{0};
  pflib::lpGBT* lpgbt_ec{0};
  pflib::zcu::OptoLink* olink{0};
  pflib::power_ctl_mezz* p_ctl{0};
};

using tool = pflib::menu::Menu<ToolBox*>;

void opto(const std::string& cmd, ToolBox* target) {
  static const int irx = 8;
  static const int itx = 8;

  pflib::zcu::OptoLink& olink = *(target->olink);
  if (cmd == "FULLSTATUS") {
    printf("Polarity -- TX: %d  RX: %d\n", olink.get_polarity(itx, false),
           olink.get_polarity(irx, true));
    std::map<std::string, uint32_t> info;
    info = olink.opto_status();
    printf("Optical status:\n");
    for (auto i : info) {
      printf("  %-20s : 0x%04x\n", i.first.c_str(), i.second);
    }
    info = olink.opto_rates();
    printf("Optical rates:\n");
    for (auto i : info) {
      printf("  %-20s : %.3f MHz (0x%04x)\n", i.first.c_str(), i.second / 1e3,
             i.second);
    }
  }
  if (cmd == "RESET") {
    olink.reset_link();
  }
  if (cmd == "POLARITY") {
    bool change;
    printf("Polarity -- TX: %d  RX: %d\n", olink.get_polarity(itx, false),
           olink.get_polarity(irx, true));
    change = tool::readline_bool("Change TX polarity? ", false);
    if (change) olink.set_polarity(!olink.get_polarity(itx, false), itx, false);
    change = tool::readline_bool("Change RX polarity? ", false);
    if (change) olink.set_polarity(!olink.get_polarity(irx, true), irx, true);
  }
  if (cmd == "LINKTRICK") {
    target->lpgbt->write(0x128, 0x5);
    sleep(1);
    target->lpgbt->write(0x128, 0x0);
  }
}

void i2c(const std::string& cmd, ToolBox* target) {
  static int ibus = 0;
  ibus = tool::readline_int("Which I2C bus?", ibus);
  target->lpgbt->setup_i2c(ibus, 100);
  if (cmd == "SCAN") {
    for (int addr = 0; addr < 0x80; addr++) {
      bool success = true;
      char failchar;
      target->lpgbt->start_i2c_read(ibus, addr);  // try reading a byte
      try {
        target->lpgbt->i2c_transaction_check(ibus, true);
      } catch (pflib::Exception& e) {
        if (e.name() == "I2CErrorNoCLK")
          failchar = 'C';
        else if (e.name() == "I2CErrorNoACK")
          failchar = '-';
        else if (e.name() == "I2CErrorSDALow")
          failchar = '*';
        else
          failchar = '?';
        //	printf(e.what());
        success = false;
      }
      if ((addr % 0x10) == 0) printf("\n%02x ", addr);
      if (success)
        printf("%02x ", addr);
      else
        printf("%c%c ", failchar, failchar);
    }
    printf("\n");
  }
  static int i2c_addr = 0;
  if (cmd == "WRITE") {
    std::vector<uint8_t> values;
    i2c_addr = tool::readline_int("What I2C Address?", i2c_addr);
    int nvalues = tool::readline_int("How many bytes?", 1);
    for (int i = 0; i < nvalues; i++) {
      char prompt[10];
      snprintf(prompt, 10, "[%d] ", i);
      values.push_back(tool::readline_int(prompt, 0));
    }
    target->lpgbt->i2c_write(ibus, i2c_addr, values);
    target->lpgbt->i2c_transaction_check(ibus, true);
  }
  if (cmd == "READ") {
    std::vector<uint8_t> values;
    i2c_addr = tool::readline_int("What I2C Address?", i2c_addr);
    int nvalues = tool::readline_int("How many bytes?", 1);
    target->lpgbt->start_i2c_read(ibus, i2c_addr, nvalues);
    target->lpgbt->i2c_transaction_check(ibus, true);
    values = target->lpgbt->i2c_read_data(ibus);
    for (int i = 0; i < nvalues; i++) {
      printf("%2d %02x (%d)\n", i, values[i], values[i]);
    }
  }
}

void general(const std::string& cmd, ToolBox* target) {
  bool comm_is_i2c = (target->lpgbt == target->lpgbt_i2c);

  if (cmd == "STATUS") {
    if (comm_is_i2c)
      printf(" Communication by I2C\n");
    else if (target->lpgbt == target->lpgbt_ic)
      printf(" Communication by IC\n");
    else
      printf(" Communication by EC\n");
    int pusm = target->lpgbt->status();
    printf(" PUSM %s (%d)\n", target->lpgbt->status_name(pusm).c_str(), pusm);
  }
  if (cmd == "RESET") {
    if (target->lpgbt_i2c != 0) {
      LPGBT_Mezz_Tester tester(target->olink->coder());
      tester.reset_lpGBT();
    } else {  // resets the TRIGGER lpGBT
      target->lpgbt_ic->gpio_set(11, false);
      target->lpgbt_ic->gpio_set(11, true);
    }
  }
  if (cmd == "EXPERT_STANDARD_HCAL_DAQ" || cmd == "STANDARD_HCAL") {
    pflib::lpgbt::standard_config::setup_hcal_daq(*target->lpgbt_ic);
    printf("Applied standard HCAL DAQ configuration\n");
  }
  if (cmd == "EXPERT_STANDARD_HCAL_TRIG" || cmd == "STANDARD_HCAL") {
    pflib::lpgbt::standard_config::setup_hcal_trig(*target->lpgbt_ec);
    printf("Applied standard HCAL TRIG configuration\n");
  }
  if (cmd == "MODE") {
    LPGBT_Mezz_Tester tester(target->olink->coder());
    printf("MODE1 = 1 for Transceiver, MODE1=0 for Transmit-only\n");
    bool wasMode, wasAddr;
    tester.get_mode(wasAddr, wasMode);
    bool newaddr = tool::readline_bool("ADDR bit", wasAddr);
    bool newmode = tool::readline_bool("MODE1 value", wasMode);
    if (newaddr != wasAddr || newmode != wasMode) {
      printf(
          "Setting new addr and mode.  lpGBT must be reset and so must this "
          "program\n\n");
      tester.set_mode(newaddr, newmode);
      tester.reset_lpGBT();
      exit(0);
    }
  }
  if (cmd == "COMM") {
    bool go_opto =
        tool::readline_bool("Use optical communication?", !comm_is_i2c);
    if (go_opto) {
      bool be_ic = tool::readline_bool("Talk to DAQ lpGBT?",
                                       target->lpgbt == target->lpgbt_ic);
      if (!be_ic) {
        printf(" Communication path is EC\n");
        target->lpgbt = target->lpgbt_ec;
      } else {
        printf(" Communication path is IC\n");
        target->lpgbt = target->lpgbt_ic;
      }
    } else {
      printf(" Communication path is wired I2C (FMC)\n");
      target->lpgbt = target->lpgbt_i2c;
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
      int adc = target->lpgbt->adc_read(whichp, 15, 1);
      float val = adc / 1024.0;
      float gain = 1;
      if (whichp >= 12) gain = 2.33645;
      printf("  ADC chan %d = %03x  (%.3f raw, %.3f cal)\n", whichp, adc, val,
             val * gain);
    }
  }
}

void elink(const std::string& cmd, ToolBox* target) {
  if (cmd == "SPY") {
    LPGBT_Mezz_Tester mezz(target->olink->coder());
    static bool isrx = false;
    isrx=tool::readline_bool("Spy on an RX? (false for TX) ", isrx);
    static int ilink = 0;
    ilink = tool::readline_int("Which elink to spy", ilink);
    std::vector<uint32_t> words = mezz.capture(ilink, isrx);
    for (size_t i = 0; i < words.size(); i++) printf("%3d %08x\n", i, words[i]);
  }
  if (cmd == "ECSPY") {
    int magic = tool::readline_int("Magic for setup", 0);
    if (magic == 1) {
      ::pflib::UIO& raw = target->olink->coder();
      raw.write(67, 0 << 8);            // disable external
      raw.write(69, 0 << (8 + 3 + 4));  // disable prbs and clear
    }
    if (magic == 2) {
      ::pflib::UIO& raw = target->olink->coder();
      raw.write(67, 1 << 8);            // enable external
      raw.write(69, 0 << (8 + 3 + 4));  // disable prbs and clear
    }
    int imode = tool::readline_int("Which mode (zero for immediate)", 0);
    std::vector<uint8_t> tx, rx;
    target->olink->capture_ec(imode, tx, rx);
    std::string stx, srx;
    for (size_t i = 0; i < tx.size(); i++) {
      stx += (tx[i] & 0x2) ? ("1") : ("0");
      stx += (tx[i] & 0x1) ? ("1") : ("0");
      srx += (rx[i] & 0x2) ? ("1") : ("0");
      srx += (rx[i] & 0x1) ? ("1") : ("0");
      if (((i + 1) % 32) == 0) {
        printf("%3d %s %s\n", i - 31, stx.c_str(), srx.c_str());
        stx = "";
        srx = "";
      }
    }
  }
  if (cmd == "ICSPY") {
    int imode = tool::readline_int("Which mode (zero for immediate)", 0);
    std::vector<uint8_t> tx, rx;
    target->olink->capture_ic(imode, tx, rx);
    std::string stx, srx;
    for (size_t i = 0; i < tx.size(); i++) {
      stx += (tx[i] & 0x2) ? ("1") : ("0");
      stx += (tx[i] & 0x1) ? ("1") : ("0");
      srx += (rx[i] & 0x2) ? ("1") : ("0");
      srx += (rx[i] & 0x1) ? ("1") : ("0");
      if (((i + 1) % 32) == 0) {
        printf("%3d %s %s\n", i - 31, stx.c_str(), srx.c_str());
        stx = "";
        srx = "";
      }
    }
  }
  if (cmd == "PATTERN") {
    bool isrx = tool::readline_bool("Adjust an RX? (false for TX) ", false);
    int ilink = tool::readline_int("Which elink to spy", 0);

    if (isrx) {
    } else {
      int mode = tool::readline_int("Change to mode",
                                    target->olink->get_elink_tx_mode(ilink));
      target->olink->set_elink_tx_mode(ilink, mode);
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

/// Walking ones
bool test_gpio(ToolBox* target) {
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
        printf("Set from lpGBT: %03x Read on I2C: %03x \n", pattern, readback2);
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
        printf("Set from lpGBT: %03x Read on I2C: %03x \n", pattern, readback2);
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
        printf("Set from I2C: %03x Read on lpGBT: %03x \n", pattern, readback);
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
        printf("Set from I2C: %03x Read on lpGBT: %03x \n", pattern, readback);
        errors++;
      }
      pattern <<= 1;
      pattern = (pattern & 0xFFF) | 1;
    }
    printf(".");
    fflush(stdout);
    cycles--;
  } while (cycles > 0 && errors == 0);
  if (errors == 0) {
    printf("\n GPIO Test was OK\n");
    return true;
  } else
    return false;
}

bool test_basic(ToolBox* target) {
  printf("Serial number: %08x\n", target->lpgbt->serial_number());
  float volts = target->p_ctl->econ_volts();
  bool volts_ok = fabs(volts - 1.2) < 0.25;
  float mA = target->p_ctl->econ_current_mA();
  bool mA_ok = (fabs(mA - 150) < 50);

  printf("lpGBT VOLTS: %0.2fV   %s\n", volts,
         (volts_ok) ? ("OK") : ("OUT-OF-RANGE"));
  printf("lpGBT current: %0.2f mA   %s\n", mA,
         (mA_ok) ? ("OK") : ("OUT-OF-RANGE"));

  return volts_ok && mA_ok;
}

bool test_adc(ToolBox* target) {
  pflib::AD5593R stim("/dev/i2c-23", 0x10);
  for (int i = 0; i < 8; i++) {
    stim.setup_dac(i);
    stim.dac_write(i, 0);
  }
  uint16_t onevolt = uint16_t(0xfff / 2.5 * 1.0);

  target->lpgbt->write(0x1d, 0x80);  // setup the standard offset

  // get the pedestal, DACs set to zero at this point
  int pedestal = target->lpgbt->adc_read(0, 15, 1);

  if (pedestal < 10 || pedestal > 50) {
    printf("Pedestal of lpGBT ADC (%d) is out of acceptable range\n", pedestal);
    //    return false;
  }

  stim.dac_write(0, onevolt);
  stim.dac_write(1, onevolt);
  int fullrange = target->lpgbt->adc_read(0, 15, 1);

  if (fullrange < 0x3D0 || fullrange > 0x3FD) {
    printf("One volt range of lpGBT ADC (%d/0x%x) is out of acceptable range\n",
           fullrange, fullrange);
    //    return false;
  }

  printf("Pedestal: %d  Full range: %d\n", pedestal, fullrange);
  double scale = 1.0 / (fullrange - pedestal);
  //  double scale = 1.04292e-03; // average lpGBT scale
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
        /// resistor chain pta -> 200 -> ch 0 -> 100 -> ch 1 -> 100 -> ch 2 ->
        /// 100 -> ch 3 -> 200 -> ptb
        double curr = (Vb - Va) / (200 * 2 + 100 * 3);
        //	  for (int i=0+4*half; i<4+4*half; i++) {
        for (int i = 0 + 4 * half; i < 4 + 4 * half; i++) {
          double expected = Va + curr * 200 + curr * 100 * (i - 4 * half);
          if (half && i == 4) {  // resistor swap on test board
            expected = Va + curr * 100 + curr * 100 * (i - 4 * half);
          }
          int adc = target->lpgbt->adc_read(i, 15, 1);
          double volts = (adc - pedestal) * scale;
          //	  double volts = (adc) * scale - 3.3542e-02;
          double error = expected - volts;
          // due to resistor chain, compliance isn't perfect when current flow
          // is non-trivial.  Allowable error scales with deltaV as a result
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
  for (int i = 0; i < 4; i++) stim.clear_pin(i);
  if (errors == 0) {
    printf(" ADC TEST PASS\n");
    return true;
  } else {
    printf(" ADC TEST ERRORS : %d\n", errors);
    return false;
  }
}

bool test_ctl(ToolBox* target) {
  LPGBT_Mezz_Tester mezz(target->olink->coder());
  printf("CTL Elink PRBS scan\n");
  std::vector<int> nok(7, 0), nbad(7, 0);
  // ensure that the lpGBT is setup properly for transmitting the fast control
  for (int i = 0; i < 7; i++) target->lpgbt->setup_etx(i, true);
  // set all the CTL links to mode (4) (just 4 links in this counting...)
  for (int i = 0; i < 4; i++) target->olink->set_elink_tx_mode(i, 4);

  bool ok = true;
  for (int ilink = 0; ilink < 7; ilink++) {
    std::vector<uint32_t> words = mezz.capture(ilink, false);
    if (words[0] == 0) {
      printf("  CTL Link %d is all zeros\n", ilink);
      ok = false;
    }
  }
  if (!ok) return false;

  // set the prbs length
  mezz.set_prbs_len_ms(1000);  // one second per point...
  for (int ph = 0; ph < 510; ph += 20) {
    mezz.set_phase(ph);
    printf("  %5d ", ph);
    std::vector<uint32_t> errs = mezz.ber_tx();
    for (int i = 0; i < int(errs.size()); i++) {
      printf("%09d ", errs[i]);
      if (errs[i] == 0)
        nok[i]++;
      else
        nbad[i]++;
    }
    printf("\n");
  }

  bool pass = true;
  for (int i = 0; i < 7; i++) {
    if (nbad[i] > 4) {
      printf(" High failure count (%d) on link %d\n", nbad[i], i);
      pass = false;
    }
  }
  if (pass)
    printf(" CTL Elink test PASSED\n");
  else
    printf(" CTL Elink test FAILED\n");
  return pass;
}

bool test_ec(ToolBox* target) {
  // for some reason, the in-firmware tester isn't working correcly now.  We are
  // using the capture as a weak replacement for now
  printf("EC PRBS scan\n");

  LPGBT_Mezz_Tester mezz(target->olink->coder());
  // very very magic
  ::pflib::UIO& raw = target->olink->coder();

  target->lpgbt->setup_ec(true, 4, true, 0);  // need to invert one or the other

  mezz.set_prbs_len_ms(1000);       // one second per point...
  raw.write(67, 1 << 8);            // enable external
  raw.write(69, 3 << (8 + 3 + 4));  // enable prbs and clear
  int nbad = 0;
  uint32_t ones = 0;

  for (int i = 0; i < 8; i++) {
    target->lpgbt->setup_ec(true, 4, true, i);

    std::vector<uint8_t> tx, rx;
    uint32_t errors = 0;

    for (int cycle = 0; cycle < 100; cycle++) {
      target->olink->capture_ec(0, tx, rx);

      std::vector<bool> srx;
      for (size_t i = 0; i < tx.size(); i++) {
        srx.push_back(rx[i] & 0x2);
        srx.push_back(rx[i] & 0x1);
      }
      for (size_t i = 7; i < srx.size(); i++) {
        bool exp = srx[i - 7] ^ srx[i - 6];
        if (exp != srx[i]) errors++;
        if (srx[i]) ones++;
      }
    }
    printf("  %d %d\n", i, errors);
    if (errors > 0) nbad++;
  }
  if (ones == 0) {
    printf("EC: Never saw any one bits -- test invalid\n");
    return false;
  } else if (nbad > 2) {
    printf("EC: Too many bad phases (%d)\n", nbad);
    return false;
  }
  return true;
}

bool test_elinks(ToolBox* target) {
  LPGBT_Mezz_Tester mezz(target->olink->coder());
  printf("Data/Trigger Elink PRBS scan\n");
  std::vector<int> nok(6, 0), nbad(6, 0);
  // ensure that the lpGBT is setup properly for receiving the data
  for (int i = 0; i < 6; i++) {
    target->lpgbt->setup_erx(i, 0, 0, 3, true);
    // set all the transmitters to mode (1)
    mezz.set_uplink_pattern(i, 1);
  }

  // set the prbs length
  mezz.set_prbs_len_ms(1000);  // one second per point...

  for (int ph = 0; ph < 500; ph += 10) {
    mezz.set_phase(ph);
    printf("  %5d ", ph);
    std::vector<uint32_t> errs = mezz.ber_rx();
    for (int i = 0; i < int(errs.size()); i++) {
      printf("%010d ", errs[i]);
      if (errs[i] == 0)
        nok[i]++;
      else
        nbad[i]++;
    }
    printf("\n");
  }
  bool pass = true;
  for (int i = 0; i < 6; i++) {
    if (nbad[i] == 0) {
      printf(" Suspiciously, never saw a failure on %d, was PRBS ok?\n", i);
      pass = false;
    } else if (nbad[i] > 18) {
      printf(" High failure count (%d) on link %d\n", nbad[i], i);
      pass = false;
    }
  }
  if (pass)
    printf(" Data/Trig Elink test PASSED\n");
  else
    printf(" Data/Trig Elink test FAILED\n");
  return pass;
}

bool test_eclk(ToolBox* target) {
  LPGBT_Mezz_Tester mezz(target->olink->coder());
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
  if (errors != 0) {
    printf("  ECLK TEST FAILED\n");
    return true;
  } else {
    printf("  ECLK TEST PASS\n");
    return false;
  }
}

void test(const std::string& cmd, ToolBox* target) {
  if (cmd == "GPIO") {
    test_gpio(target);
  }
  if (cmd == "BASIC") {
    test_basic(target);
  }
  if (cmd == "ADC") {
    test_adc(target);
  }
  if (cmd == "ECLK") {
    test_eclk(target);
  }
  if (cmd == "CTL") {
    test_ctl(target);
  }
  if (cmd == "ELINKS") {
    test_elinks(target);
  }
  if (cmd == "EC") {
    test_ec(target);
  }
  if (cmd == "ALL") {
    test_basic(target);
    test_gpio(target);
    test_eclk(target);
    test_ctl(target);
    test_elinks(target);
    test_ec(target);
    test_adc(target);
  }
}

namespace {

auto gen =
    tool::menu("GENERAL", "GENERAL funcations")
        ->line("STATUS", "Status summary", general)
        ->line("MODE", "Setup the lpGBT ADDR and MODE1", general)
        ->line("STANDARD_HCA", "Apply standard HCAL lpGBT setups", general)
        ->line("EXPERT_STANDARD_HCAL_DAQ",
               "Apply just standard HCAL DAQ lpGBT setup", general)
        ->line("EXPERT_STANDARD_HCAL_TRIG",
               "Apply just standard HCAL TRIG lpGBT setup", general)
        ->line("RESET", "Reset the lpGBT", general)
        ->line("COMM", "Communication mode", general);

auto optom =
    tool::menu("OPTO", "Optical Link Functions")
        ->line("FULLSTATUS", "Get full status", opto)
        ->line("RESET", "Reset optical link", opto)
        ->line("POLARITY", "Adjust the polarity", opto)
        ->line("LINKTRICK", "Cycle into/out of fixed speed to get SFP to lock",
               opto);

auto direct = tool::menu("REG", "Direct Register Actions")
                  ->line("READ", "Read one or several registers", regs)
                  ->line("WRITE", "Write a register", regs)
                  ->line("LOAD", "Load from a CSV file", regs);

auto mgpio = tool::menu("GPIO", "GPIO controls")
                 ->line("SET", "Set a GPIO pin", gpio)
                 ->line("CLEAR", "CLEAR a GPIO pin", gpio)
                 ->line("WRITE", "Write all GPIO pins at once", gpio);

auto melink = tool::menu("ELINK", "Elink-related items")
                  ->line("SETUP", "Setup a pin", elink)
                  ->line("PATTERN", "Pattern selection for", elink)
                  ->line("SPY", "Spy on one or more pins", elink)
                  ->line("ECSPY", "Spy on the EC link", elink)
                  ->line("ICSPY", "Spy on the IC link", elink);

auto madc = tool::menu("ADC", "ADC and DAC-related actions")
                ->line("READ", "Read an ADC line", adc)
                ->line("ALL", "Read all ADC lines", adc);

auto mi2c = tool::menu("I2C", "I2C activities")
                ->line("SCAN", "Scan an I2C bus", i2c)
                ->line("READ", "Read from an I2C device", i2c)
                ->line("WRITE", "Write to an I2C device", i2c);

auto mtest =
    tool::menu("TEST", "Mezzanine testing functions")
        ->line("BASIC", "Test the power and communication functions", test)
        ->line("GPIO", "Test the gpio functions", test)
        ->line("ADC", "Test the ADC function", test)
        ->line("ECLK", "Test the elocks", test)
        ->line("CTL", "Test the fast control lines", test)
        ->line("EC", "Test the EC control link", test)
        ->line("ELINKS", "Test the uplink elinks", test)
        ->line("ALL", "Run all tests", test);

}  // namespace

int main(int argc, char* argv[]) {
  if (argc == 2 and (!strcmp(argv[1], "-h") or !strcmp(argv[1], "--help"))) {
    printf("\nUSAGE: \n");
    printf("   %s OPTIONS\n\n", argv[0]);
    printf("OPTIONS:\n");
    printf(
        "  --do [number] : Dual-optical configuration, implies no mezzanine\n");
    printf("  -o : Use optical communication by default\n");
    printf("  --nm : No mezzanine\n");
    printf("  -s [file] : pass a script of commands to run through\n");
    printf("  -h|--help : print this help and exit\n");
    printf("\n");
    return 1;
  }

  bool wired = true;
  bool nomezz = false;
  std::string target_name("singleLPGBT");

  for (int i = 1; i < argc; i++) {
    std::string arg(argv[i]);
    if (arg == "-o") wired = false;
    if (arg == "--nm") nomezz = true;
    if (arg == "--do") {
      wired = false;
      nomezz = true;
      i++;
      target_name = "standardLpGBTpair-";
      target_name += argv[i][0];
      printf("%s\n", target_name.c_str());
    }

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

  tool::set_history_filepath("~/.pflpgbt-history");

  ToolBox t;

  pflib::zcu::OptoLink olink(target_name.c_str());
  int chipaddr = 0x78;
  if (wired) {
    LPGBT_Mezz_Tester tester(olink.coder());
    bool addr, mode1;
    tester.get_mode(addr, mode1);  // need to determine address
    if (addr) chipaddr |= 0x1;
    if (mode1) chipaddr |= 0x4;
    printf(" ADDR = %d and MODE1=%d -> 0x%02x\n", addr, mode1, chipaddr);
    pflib::lpGBT_ConfigTransport_I2C* tport =
        new pflib::lpGBT_ConfigTransport_I2C(chipaddr, "/dev/i2c-23");
    t.lpgbt_i2c = new pflib::lpGBT(*tport);
  } else {
    chipaddr |= 0x4;
  }

  pflib::zcu::lpGBT_ICEC_Simple ic(target_name, false, chipaddr);
  pflib::lpGBT lpgbt_ic(ic);
  pflib::zcu::lpGBT_ICEC_Simple ec(target_name, true,
                                   0x78);  // correct for HCAL
  pflib::lpGBT lpgbt_ec(ec);
  tool::set_history_filepath("~/.pflpgbt-history");
  t.lpgbt_ic = &lpgbt_ic;
  t.lpgbt_ec = &lpgbt_ec;
  if (wired)
    t.lpgbt = t.lpgbt_i2c;
  else
    t.lpgbt = t.lpgbt_ic;
  t.olink = &olink;

  /// need to make sure the voltage is at a safe level, will be done
  /// automatically here
  pflib::power_ctl_mezz* ctl(0);
  if (!nomezz) ctl = new pflib::power_ctl_mezz("/dev/i2c-23");
  t.p_ctl = ctl;

  tool::run(&t);

  return 0;
}
