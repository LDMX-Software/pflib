#include "pflib/lpgbt/lpGBT_ConfigTransport_I2C.h"
#include "pflib/lpgbt/lpGBT_Utility.h"
#include <fstream>
#include <iostream>
#include "Menu.h"

using tool = Menu<pflib::lpGBT>;

void regs(const std::string& cmd, pflib::lpGBT* target ) {
  static int addr = 0;
  if (cmd=="READ") {
    addr = BaseMenu::readline_int("Register",addr,true);
    int n  = BaseMenu::readline_int("Number of items",1);
    for (int i=0; i<n; i++) {
      printf("   %03x : %02x\n",addr+i,target->read(addr+i));
    }
  }
  if (cmd=="WRITE") {
    addr = BaseMenu::readline_int("Register",addr,true);
    int value = target->read(addr);
    value = BaseMenu::readline_int("New value",value,true);
    target->write(addr,value);
  }
  if (cmd=="LOAD") {
    std::string fname = BaseMenu::readline("File: ");
    pflib::lpgbt::applylpGBTCSV(fname,*target);
  }
}

void gpio(const std::string& cmd, pflib::lpGBT* target ) {
  if (cmd=="SET") {
    int which=BaseMenu::readline_int("Pin");
    if (which>=0 && which<12) target->gpio_set(which,true);
  }
  if (cmd=="CLEAR") {
    int which=BaseMenu::readline_int("Pin");
    if (which>=0 && which<12) target->gpio_set(which,false);
  }
  if (cmd=="WRITE") {
    uint16_t value=target->gpio_get();
    value=BaseMenu::readline_int("Value",value,true);
    target->gpio_set(value);
  }
}

void adc(const std::string& cmd, pflib::lpGBT* target) {
  if (cmd=="READ") {
    int whichp=BaseMenu::readline_int("Channel (pos)");
    int whichn=BaseMenu::readline_int("Channel (neg)",15);
    int gain=BaseMenu::readline_int("Gain",1);
    printf("  ADC = %03x\n",target->adc_read(whichp,whichn,gain));
  }
  if (cmd=="ALL") {
    for (int whichp=0; whichp<15; whichp++) {
      printf("  ADC chan %d = %03x\n",whichp,target->adc_read(whichp,15,1));
    }
  }
}

static uint16_t lcl2gbt(uint16_t val) {
  uint16_t rval=0;
  if (val&(1<<0)) rval|=(1<<9);
  if (val&(1<<1)) rval|=(1<<3);
  if (val&(1<<2)) rval|=(1<<6);
  if (val&(1<<3)) rval|=(1<<7);
  if (val&(1<<4)) rval|=(1<<1);
  if (val&(1<<5)) rval|=(1<<0);
  if (val&(1<<6)) rval|=(1<<2);
  if (val&(1<<7)) rval|=(1<<4);
  if (val&(1<<8)) rval|=(1<<10);
  if (val&(1<<9)) rval|=(1<<11);
  if (val&(1<<10)) rval|=(1<<8);
  if (val&(1<<11)) rval|=(1<<5);
  return rval;
}
static uint16_t gbt2lcl(uint16_t val) {
  uint16_t rval=0;
  if (val&(1<<0)) rval|=(1<<5);
  if (val&(1<<1)) rval|=(1<<4);
  if (val&(1<<2)) rval|=(1<<6);
  if (val&(1<<3)) rval|=(1<<1);
  if (val&(1<<4)) rval|=(1<<7);
  if (val&(1<<5)) rval|=(1<<11);
  if (val&(1<<6)) rval|=(1<<2);
  if (val&(1<<7)) rval|=(1<<3);
  if (val&(1<<8)) rval|=(1<<10);
  if (val&(1<<9)) rval|=(1<<0);
  if (val&(1<<10)) rval|=(1<<8);
  if (val&(1<<11)) rval|=(1<<9);
  return rval;
}
  

void test(const std::string& cmd, pflib::lpGBT* target) {
  if (cmd=="GPIO") {
    // set up the external GPIO to read mode first
    pflib::lpGBT_ConfigTransport_I2C ext(0x21,"/dev/i2c-23");
    ext.write_raw(0x8,0xff);
    ext.write_raw(0x9,0x0f);
    // setup the internal GPIO as write mode
    for (int i=0; i<12; i++)
      target->gpio_cfg_set(i,1); // output
    uint16_t pattern=0x1;
    for (int i=0; i<12; i++) {
      target->gpio_set(pattern);
      ext.write_raw(0x80);
      uint16_t readback=(ext.read_raw() | (uint16_t(ext.read_raw())<<8))&0xFFF;
      uint16_t readback2=lcl2gbt(readback);
      printf("%03x %03x %03x\n",pattern,readback, readback2);
      pattern<<=1;
    }
  }
  
}

namespace {
  auto direct = tool::menu("REG","Direct Register Actions")
    ->line("READ","Read one or several registers", regs)
    ->line("WRITE","Write a register", regs)
    ->line("LOAD","Load from a CSV file", regs);
  

  auto mgpio = tool::menu("GPIO","GPIO controls")
    ->line("SET","Set a GPIO pin",gpio)
    ->line("CLEAR","CLEAR a GPIO pin",gpio)
    ->line("WRITE","Write all GPIO pins at once",gpio);
  
  auto madc = tool::menu("ADC","ADC and DAC-related actions")
    ->line("READ","Read an ADC line",adc)
    ->line("ALL","Read all ADC lines",adc);

}

int main(int argc, char* argv[]) {

  if (argc == 2 and (!strcmp(argv[1],"-h") or !strcmp(argv[1],"--help"))) {
    printf("\nUSAGE: \n");
    printf("   %s -z OPTIONS\n\n", argv[0]);
    printf("OPTIONS:\n");
    printf("  -s [file] : pass a script of commands to run through\n");
    printf("  -h|--help : print this help and exit\n");
    printf("\n");
    return 1;
  }

  for (int i = 1 ; i < argc ; i++) {
    std::string arg(argv[i]);
    if (arg=="-s") {
      if (i+1 == argc or argv[i+1][0] == '-') {
        std::cerr << "Argument " << arg << " requires a file after it." << std::endl;
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
        if ( !line.empty() && line[0] == '#' ) continue ;
        // add to command queue
        BaseMenu::add_to_command_queue(line);
      }
      sFile.close() ;
    }
  }

  pflib::lpGBT_ConfigTransport_I2C tport(0x79,"/dev/i2c-23");
  pflib::lpGBT lpgbt(tport);
  tool::run(&lpgbt);

  return 0;
  
}
