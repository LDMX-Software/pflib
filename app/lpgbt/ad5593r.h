#ifndef AD5593R_H_INCLUDED
#define AD5593R_H_INCLUDED

// using this transport as a general I2C interface
#include "pflib/lpgbt/lpGBT_ConfigTransport_I2C.h"

namespace pflib {

/** Partial clone from python code, with just needed functionalities for testing for now.
 */
class AD5593R {
  static constexpr uint8_t REG_DAC_PIN = 0x05;
  static constexpr uint8_t REG_ADC_PIN = 0x04;
  static constexpr uint8_t REG_OPENDRAIN = 0x0C;
  static constexpr uint8_t REG_GPO_ENABLE = 0x08;
  static constexpr uint8_t REG_GPI_ENABLE = 0x0A;
  static constexpr uint8_t REG_ADC_SEQ = 0x02;
  static constexpr uint8_t REG_GENERAL = 0x03;
  static constexpr uint8_t REG_POWERDOWN = 0x0B;
  static constexpr uint8_t REG_PULLDOWN = 0x06;
  static constexpr uint8_t REG_GPO_WRITE = 0x09;
  static constexpr uint8_t REG_GPI_READ = 0x60;
  static constexpr uint8_t REG_RESET = 0x0F;

 public:
  AD5593R(const std::string& bus, uint8_t addr) : i2c_{addr, bus} {
    // turn on the reference
    i2c_.write_raw(REG_POWERDOWN, 0x2, 0x0);
    // turn on the ADC buffer, setup range, etc
    i2c_.write_raw(REG_GENERAL, 0x1, 0x0);
  }

  void setup_gpi(int pin, bool pull_down= false) {
    if (pin < 0 || pin > 7) return;
    clear_bit(REG_GPO_ENABLE, pin);
    clear_bit(REG_ADC_PIN, pin);
    clear_bit(REG_DAC_PIN, pin);
    set_bit(REG_GPI_ENABLE, pin);
    if (pull_down) {
      set_bit(REG_PULLDOWN,pin);
    } else {
      clear_bit(REG_PULLDOWN,pin);
    }
  }

  void setup_gpo(int pin, int value=-1) {
    if (pin < 0 || pin > 7) return;
    clear_bit(REG_GPI_ENABLE, pin);
    clear_bit(REG_ADC_PIN, pin);
    clear_bit(REG_DAC_PIN, pin);
    if (value==0) {
      clear_bit(REG_GPO_WRITE,pin);
    } else {
      set_bit(REG_GPO_WRITE,pin);
    }
    set_bit(REG_GPO_ENABLE, pin);
  }

  bool is_gpo(int pin) {
    return read_reg(REG_GPO_ENABLE)&(1<<pin);
  }

  bool gpo_get_value(int pin) {
    return read_reg(REG_GPO_WRITE)&(1<<pin);
  }
  
  void setup_dac(int pin, bool zero = true) {
    if (pin < 0 || pin > 7) return;
    clear_bit(REG_GPO_ENABLE, pin);
    clear_bit(REG_GPI_ENABLE, pin);
    set_bit(REG_ADC_PIN, pin);
    set_bit(REG_PULLDOWN, pin);
    // set DAC to zero by default
    if (zero) {
      dac_write(pin, 0);
    }
    set_bit(REG_DAC_PIN, pin);
  }

  void setup_adc(int pin) {
    if (pin < 0 || pin > 7) return;
    // turn off DAC on pin
    clear_bit(REG_DAC_PIN,pin);
    // turn off gpi on pin
    clear_bit(REG_GPI_ENABLE,pin);
    // turn off gpo on pin
    clear_bit(REG_GPO_ENABLE,pin);
    // turn off pull-down on pin
    clear_bit(REG_PULLDOWN,pin);
    // enable ADC
    set_bit(REG_ADC_PIN,pin);
  }

  int adc_read(int pin) {
    //""" selects a pin (must be configured already for ADC operation), performs ADC conversion sequence and reads back the raw 12 bit result from the ADC data register for that pin """
    // first byte sets repetition and temperature indicator readback, second byte selects ADC channel (all off by default)
    i2c_.write_raw(REG_ADC_SEQ,0x0,(1<<pin));
    // select ADC address
    i2c_.write_raw(0x40);
    std::vector<uint8_t> result=i2c_.read_raw(2);
    return int(result[1]) + (int(result[0]&0xF)<<8);
  }

  float adc_volts(int pin, int ave=10) {
    int sum=0;
    for (int i = 0; i<ave; i++)
      sum+=adc_read(pin);
    return (sum*1.0/ave)*2.5/4096;
  }

  
  void dac_write(int pin, uint16_t value) {
    if (pin < 0 || pin > 7) return;
    i2c_.write_raw(0x10 | pin, ((0x08 | pin) << 4) | ((value & 0xF00) >> 8),
                   value & 0xFF);
  }

  void clear_pin(int pin) {
    clear_bit(REG_GPO_ENABLE, pin);
    clear_bit(REG_GPI_ENABLE, pin);
    clear_bit(REG_ADC_PIN, pin);
    clear_bit(REG_DAC_PIN, pin);
  }

  

 private:
  uint16_t read_reg(uint8_t reg) {
    i2c_.write_raw(0x70 | (reg & 0xF));
    std::vector<uint8_t> rv = i2c_.read_raw(2);
    return (uint16_t(rv[0]) << 8) | (rv[1]);
  }
  void write_reg(uint8_t reg, uint16_t value) {
    uint8_t b = uint8_t(value >> 8);
    uint8_t c = uint8_t(value & 0xFF);
    i2c_.write_raw((reg & 0xF), b, c);
  }
  void set_bit(uint8_t reg, int bit) {
    uint16_t val = read_reg(reg);
    val |= (1 << bit);
    write_reg(reg, val);
  }
  void clear_bit(uint8_t reg, int bit) {
    uint16_t val = read_reg(reg);
    val |= (1 << bit);
    val ^= (1 << bit);
    write_reg(reg, val);
  }

  lpGBT_ConfigTransport_I2C i2c_;
};
  
}  // namespace pflib

#endif  // AD5593R_H_INCLUDED
