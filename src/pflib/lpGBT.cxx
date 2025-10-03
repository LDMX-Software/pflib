#include "pflib/lpGBT.h"

#include <unistd.h>

namespace pflib {

std::vector<uint8_t> lpGBT_ConfigTransport::read_regs(uint16_t reg, int n) {
  std::vector<uint8_t> v;
  for (int i = 0; i < n; i++) v.push_back(read_reg(reg + i));
  return v;
}
void lpGBT_ConfigTransport::write_regs(uint16_t reg,
                                       const std::vector<uint8_t>& value) {
  for (size_t i = 0; i < value.size(); i++) write_reg(i + reg, value[i]);
}

lpGBT::lpGBT(lpGBT_ConfigTransport& transport) : tport_{transport}, gpio_{*this} {}

void lpGBT::write(const RegisterValueVector& regvalues) {
  std::vector<uint8_t> tosend;
  static const uint16_t UNDEF_ADDR = 0xFFFF;
  uint16_t base_addr = UNDEF_ADDR;

  // slightly complex, to allow batching of writes to optimize I/O usage

  for (size_t ptr = 0; ptr < regvalues.size(); ptr++) {
    if (ptr + 1 < regvalues.size() &&
        regvalues[ptr + 1].first ==
            regvalues[ptr].first +
                1) {  // we will start or continue to build a sequence...
      if (base_addr == UNDEF_ADDR)
        base_addr = regvalues[ptr].first;  // start a new sequence
      tosend.push_back(regvalues[ptr].second);
    } else if (
        base_addr !=
        UNDEF_ADDR) {  // this means the current value must be in sequence, even though the next one isn't
      tosend.push_back(regvalues[ptr].second);
      tport_.write_regs(base_addr, tosend);
      tosend.clear();
      base_addr = UNDEF_ADDR;
    } else {  // we are a one-off
      tport_.write_reg(regvalues[ptr].first, regvalues[ptr].second);
    }
  }
}

lpGBT::RegisterValueVector lpGBT::read(const std::vector<uint16_t>& registers) {
  RegisterValueVector retval;

  static const uint16_t UNDEF_ADDR = 0xFFFF;
  uint16_t base_addr = UNDEF_ADDR;
  int n = -1;

  // slightly complex, to allow batching of reads to optimize I/O usage

  for (size_t ptr = 0; ptr < registers.size(); ptr++) {
    if (ptr + 1 < registers.size() &&
        registers[ptr + 1] ==
            registers[ptr] +
                1) {  // we will start or continue to build a sequence...
      if (base_addr == UNDEF_ADDR) {
        base_addr = registers[ptr];  // start a new sequence
        n = 0;
      }
      n++;
    } else if (
        base_addr !=
        UNDEF_ADDR) {  // this means the current register must be in sequence, even though the next one isn't
      n++;
      std::vector<uint8_t> values = tport_.read_regs(base_addr, n);
      for (int i = 0; i < n; i++)
        retval.push_back(RegisterValue(base_addr + i, values[i]));
      base_addr = UNDEF_ADDR;
    } else {  // we are a one-off
      retval.push_back(
          RegisterValue(registers[ptr], tport_.read_reg(registers[ptr])));
    }
  }
  return retval;
}

void lpGBT::bit_set(uint16_t reg, int ibit) {
  uint8_t cval = tport_.read_reg(reg);
  cval |= (1 << (ibit % 8));
  tport_.write_reg(reg, cval);
}
void lpGBT::bit_clr(uint16_t reg, int ibit) {
  uint8_t cval = tport_.read_reg(reg);
  cval |= (1 << (ibit % 8));
  cval ^= (1 << (ibit % 8));
  tport_.write_reg(reg, cval);
}

/// Register constants here are all correct for V1 and V2 lpGBTs
static constexpr uint16_t REG_PIODIRH = 0x053;
static constexpr uint16_t REG_PIOPULLENAH = 0x057;
static constexpr uint16_t REG_PIOUPDOWNH = 0x059;
static constexpr uint16_t REG_PIODRIVEH = 0x05b;

static constexpr uint16_t REG_PIOOUTH = 0x055;
static constexpr uint16_t REG_PIOOUTL = 0x056;
static constexpr uint16_t REG_PIOINH = 0x1AF;
static constexpr uint16_t REG_PIOINL = 0x1B0;

static constexpr uint16_t REG_VREFCNTR = 0x01c;
static constexpr uint16_t REG_DAC_CONFIG_H = 0x06a;
static constexpr uint16_t REG_CURDAC_VALUE = 0x06c;
static constexpr uint16_t REG_CURDAC_CHN = 0x06d;
static constexpr uint16_t REG_ADC_SELECT = 0x121;
static constexpr uint16_t REG_ADCMON = 0x122;
static constexpr uint16_t REG_ADC_CONFIG = 0x123;
static constexpr uint16_t REG_ADC_STATUS_H = 0x1ca;
static constexpr uint16_t REG_ADC_STATUS_L = 0x1cb;

static constexpr uint16_t REG_EPTXDATARATE = 0x0a8;
static constexpr uint16_t REG_EPTXCONTROL = 0x0a9;
static constexpr uint16_t REG_EPTX10ENABLE = 0x0aa;
static constexpr uint16_t REG_EPTXECCHNCNTR = 0x0ac;
static constexpr uint16_t REG_EPTX00ChnCntr = 0x0ae;
static constexpr uint16_t REG_EPTX01_00ChnCntr = 0x0be;
static constexpr uint16_t REG_EPRX0CONTROL = 0x0c8;
static constexpr uint16_t REG_EPRX00CHNCNTR = 0x0d0;
static constexpr uint16_t REG_ECLK_BASE = 0x06e;
static constexpr uint16_t REG_POWERUP_STATUS = 0x1d9;

static constexpr uint16_t REG_I2CM0CONFIG   = 0x100;
static constexpr uint16_t REG_I2CM0ADDRESS  = 0x101;
static constexpr uint16_t REG_I2CM0DATA0    = 0x102;
static constexpr uint16_t REG_I2CM0CMD      = 0x106;
static constexpr uint16_t REG_I2CM0STATUS   = 0x171;
static constexpr uint16_t REG_I2CM0READBYTE = 0x173;
static constexpr uint16_t REG_I2CM0READ0    = 0x174;
static constexpr uint16_t REG_I2CM0READ15   = 0x183;
static constexpr uint16_t REG_I2C_WSTRIDE   = 7;
static constexpr uint16_t REG_I2C_RSTRIDE   = 21;
  
static constexpr uint16_t REG_FUSECONTROL = 0x119;
static constexpr uint16_t REG_FUSEADDRH = 0x11E;
static constexpr uint16_t REG_FUSEADDRL = 0x11F;
static constexpr uint16_t REG_FUSESTATUS = 0x1B1;
static constexpr uint16_t REG_FUSEREADA = 0x1B2;

static uint32_t rotate_right(uint32_t value, int shift) {
  return (value >> shift) | ((value << (32 - shift)) & 0xFFFFFFFFu);
}

uint32_t lpGBT::read_efuse(uint16_t reg) {
  bit_set(REG_FUSECONTROL, 1);  // FuseRead
  while (!(read(REG_FUSESTATUS) & (1 << 2))) {
    usleep(1);
  }
  // wait for ready
  write(REG_FUSEADDRH, uint8_t(reg >> 8));
  write(REG_FUSEADDRL, uint8_t(reg));
  uint32_t retval(0);
  for (int i = 4; i > 0; i--) {
    retval = retval << 8;
    retval = retval | read(REG_FUSEREADA + i - 1);
  }
  bit_clr(REG_FUSECONTROL, 1);  // FuseRead
  bit_clr(REG_FUSECONTROL, 0);  // FuseRead
  return retval;
}

uint32_t lpGBT::serial_number() {
  const int points[] = {0, 0, 0x8, 6, 0xC, 12, 0x10, 18, 0x14, 24};
  uint32_t results[5];
  for (int i = 0; i < 5; i++) {
    // fuse read
    results[i] = read_efuse(points[i * 2]);
    uint32_t raw = results[i];
    // rotate
    results[i] = rotate_right(results[i], points[i * 2 + 1]);
    //    printf("0x%08x 0x%0x\n",raw,results[i]);
  }
  if (results[1] == 0 && results[2] == 0 && results[3] == 0 &&
      results[4] == 0) {  // no redundency
    return results[0];
  } else {
    uint32_t voted = 0;
    for (int i = 0; i < 32; i++) {
      int nones = 0;
      for (int j = 0; j < 5; j++)
        if (results[j] & (1 << i)) nones++;
      if (nones >= 3) voted |= (1 << i);
      //      if (nones!=0 && nones!=5) printf("Disagreement in bit %d\n",i);
    }
    return voted;
  }
}

void lpGBT::gpio_set(int ibit, bool high) {
  if (ibit < 0 || ibit > 15) {
    char msg[100];
    snprintf(msg, 100, "GPIO bit %d is out of range 0:15", ibit);
    PFEXCEPTION_RAISE("OutOfRangeException", msg);
  }
  if (ibit == 3) ibit = 15;  // yes this is strange
  uint16_t reg = (ibit < 8) ? (REG_PIOOUTL) : (REG_PIOOUTH);
  if (high)
    bit_set(reg, ibit % 8);
  else
    bit_clr(reg, ibit % 8);
}

bool lpGBT::gpio_get(int ibit) {
  if (ibit < 0 || ibit > 15) {
    char msg[100];
    snprintf(msg, 100, "GPIO bit %d is out of range 0:15", ibit);
    PFEXCEPTION_RAISE("OutOfRangeException", msg);
  }
  if (ibit == 3) ibit = 15;  // yes this is strange
  uint16_t reg = (ibit < 8) ? (REG_PIOINL) : (REG_PIOINH);
  uint8_t value = tport_.read_reg(reg);
  return (value & (1 << (ibit & 8)));
}

void lpGBT::gpio_set(uint16_t values) {
  std::vector<uint8_t> vals;
  vals.push_back(uint8_t(values >> 8) | ((values & 0x8) << 4));
  vals.push_back(uint8_t(values & 0xFF));
  tport_.write_regs(REG_PIOOUTH, vals);
}

uint16_t lpGBT::gpio_get() {
  std::vector<uint8_t> vals = tport_.read_regs(REG_PIOINH, 2);
  uint16_t val = uint16_t(vals[1]) | ((uint16_t(vals[0])) << 8);
  val = (val & 0xFF7) | (val & 0x8000) >> (12);
  return val;
}

int lpGBT::gpio_cfg_get(int ibit) {
  int cfg = 0;
  if (ibit < 0 || ibit > 15) {
    char msg[100];
    snprintf(msg, 100, "GPIO bit %d is out of range 0:15", ibit);
    PFEXCEPTION_RAISE("OutOfRangeException", msg);
  }
  if (ibit == 3) ibit = 15;  // yes this is strange

  int offset =
      (ibit < 8) ? (1) : (0);  // fixed relationship between H and L bytes
  if (read(REG_PIODIRH + offset)) cfg |= GPIO_IS_OUTPUT;
  if (read(REG_PIODRIVEH + offset)) cfg |= GPIO_IS_STRONG;
  if (read(REG_PIOPULLENAH + offset)) {
    if (read(REG_PIOUPDOWNH + offset))
      cfg |= GPIO_IS_PULLUP;
    else
      cfg |= GPIO_IS_PULLDOWN;
  }
  return cfg;
}

void lpGBT::gpio_cfg_set(int ibit, int cfg, const std::string& name) {
  if (ibit < 0 || ibit > 15) {
    char msg[100];
    snprintf(msg, 100, "GPIO bit %d is out of range 0:15", ibit);
    PFEXCEPTION_RAISE("OutOfRangeException", msg);
  }
  if (ibit == 3) ibit = 15;  // yes this is strange
  int offset =
      (ibit < 8) ? (1) : (0);  // fixed relationship between H and L bytes
  if (cfg & GPIO_IS_OUTPUT)
    bit_set(REG_PIODIRH + offset, ibit % 8);
  else
    bit_clr(REG_PIODIRH + offset, ibit % 8);
  if (cfg & GPIO_IS_STRONG)
    bit_set(REG_PIODRIVEH + offset, ibit % 8);
  else
    bit_clr(REG_PIODRIVEH + offset, ibit % 8);

  if ((cfg & 0x6) == GPIO_IS_PULLUP) {
    bit_set(REG_PIOUPDOWNH + offset, ibit % 8);
    bit_set(REG_PIOPULLENAH + offset, ibit % 8);
  } else if ((cfg & 0x6) == GPIO_IS_PULLDOWN) {
    bit_clr(REG_PIOUPDOWNH + offset, ibit % 8);
    bit_set(REG_PIOPULLENAH + offset, ibit % 8);
  } else {
    bit_clr(REG_PIOPULLENAH + offset, ibit % 8);
  }
  gpio_.add_pin(name,ibit,cfg&GPIO_IS_OUTPUT);
}

uint16_t lpGBT::adc_resistance_read(int ipos, int current, int gain) {
  if (ipos > 7) {
    PFEXCEPTION_RAISE(
        "ADCException",
        "Selected channel out of range for resistance measurement");
  }
  static constexpr int CURDAC_ENABLE = 6;  // bit 6

  bit_set(REG_DAC_CONFIG_H, CURDAC_ENABLE);
  tport_.write_reg(REG_CURDAC_VALUE, current & (0xff));
  tport_.write_reg(REG_CURDAC_CHN, (1 << ipos));

  uint16_t value = adc_read(ipos, 15, gain);

  bit_clr(REG_DAC_CONFIG_H, CURDAC_ENABLE);

  return value;
}

uint16_t lpGBT::adc_read(int ipos, int ineg, int gain) {
  // bit definitions
  static constexpr uint8_t M_ADC_CONFIG_CONVERT = uint8_t(1) << 7;
  static constexpr uint8_t M_ADC_CONFIG_ENABLE = uint8_t(1) << 2;
  static constexpr uint8_t M_ADC_STATUS_H_DONE = uint8_t(1) << 6;
  static constexpr uint8_t M_VREF_ENABLE = uint8_t(1) << 7;

  if (ipos >= 10 && ipos <= 13) {  // must enable the ADCMON
    tport_.write_reg(REG_ADCMON, 0x1F);
  }
  // work out the gain value
  int gval = 0;
  if (gain == 8) gval = 1;
  if (gain == 16) gval = 2;
  if (gain == 32) gval = 3;

  // set up the multiplexers
  tport_.write_reg(REG_ADC_SELECT, (ipos << 4) | (ineg));
  // enable the ADC and set the gain
  tport_.write_reg(REG_ADC_CONFIG, M_ADC_CONFIG_ENABLE | gval);
  // enable vref
  tport_.write_reg(REG_VREFCNTR, M_VREF_ENABLE);
  usleep(1000);
  // start conversion
  tport_.write_reg(REG_ADC_CONFIG,
                   M_ADC_CONFIG_CONVERT | M_ADC_CONFIG_ENABLE | gval);

  // wait and check if done
  while (!(tport_.read_reg(REG_ADC_STATUS_H) & M_ADC_STATUS_H_DONE))
    usleep(1000);

  uint16_t adc_value = ((tport_.read_reg(REG_ADC_STATUS_H) & 0x3) << 8) |
                       tport_.read_reg(REG_ADC_STATUS_L);

  // shut things down
  tport_.write_reg(REG_ADC_CONFIG, M_ADC_CONFIG_ENABLE | gain);

  if (ipos >= 10 && ipos <= 13) tport_.write_reg(REG_ADCMON, 0);
  tport_.write_reg(REG_VREFCNTR, 0);

  return adc_value;
}

void lpGBT::setup_erx(int irx, int align, int alignphase, int speed,
                      bool invert, bool term, int equalization, bool acbias) {
  if (irx < 0 || irx > 5) return;
  if (irx >= 3) irx++;  // irx=3 is EDIN4, 4->5, 5->6

  // enable first channel, set speed and alignment strategy
  tport_.write_reg(REG_EPRX0CONTROL + irx,
                   0x10 | ((speed & 0x3) << 2) | (align & 0x3));
  //
  tport_.write_reg(REG_EPRX00CHNCNTR + irx * 4,
                   ((alignphase & 0xF) << 4) | ((invert) ? (0x8) : (0)) |
                       ((acbias) ? (0x4) : (0)) | ((term) ? (0x2) : (0)));
  // ignore equalization for now
}

void lpGBT::setup_etx(int itx, bool enable, bool invert, int drive, int pe_mode,
                      int pe_strength, int pe_width) {
  if (itx < 0 || itx > 6) return;
  // 0x12 -> EGROUP 1, LINK 2
  static constexpr uint8_t MAP_ETX[7] = {0x00, 0x01, 0x10, 0x20,
                                         0x21, 0x30, 0x31};

  // always set up the data rate
  tport_.write_reg(REG_EPTXDATARATE, 0xFF);  // all links at 320 MBps
  tport_.write_reg(REG_EPTXCONTROL, 0x0F);   // enable mirroring
  int iport = (MAP_ETX[itx] >> 4);
  int ipin = (MAP_ETX[itx] & 0xF);

  if (!enable) {
    bit_clr(REG_EPTX10ENABLE + iport / 2, (iport % 2) * 4 + ipin);
    return;
  } else {
    bit_set(REG_EPTX10ENABLE + iport / 2, (iport % 2) * 4 + ipin);
  }

  uint16_t reg = REG_EPTX00ChnCntr + iport * 4 + ipin;
  tport_.write_reg(
      reg, ((pe_strength & 0x7) << 5) | ((pe_mode & 0x3) << 3) | (drive & 0x7));

  reg = REG_EPTX01_00ChnCntr + iport * 2 + ipin / 2;
  uint8_t val = tport_.read_reg(reg);
  if (ipin % 1) {
    val = val & 0x0F;
    if (invert) val |= 0x80;
    val |= (pe_width & 0x7) << 4;
  } else {
    val = val & 0xF0;
    if (invert) val |= 0x08;
    val |= (pe_width & 0x7);
  }
  tport_.write_reg(reg, val);
}

void lpGBT::setup_ec(bool invert_tx, int drive, bool fixed, int alignphase,
                     bool invert_rx, bool term, bool acbias, bool pullup) {
  // enable enable, pick fixed or not
  tport_.write_reg(REG_EPRX0CONTROL + 7, 0x10 | (fixed ? (1) : (0)));
  // phase, invert, bias, term, pullup
  tport_.write_reg(REG_EPRX00CHNCNTR + 7 * 4,
                   ((alignphase & 0x7) << 4) | ((invert_rx) ? (0x8) : (0)) |
                       ((acbias) ? (0x4) : (0)) | ((term) ? (0x2) : (0)) |
                       ((pullup) ? (0x1) : (0)));
  // TX side
  tport_.write_reg(REG_EPTXECCHNCNTR,
                   ((drive & 0x7) << 5) | ((invert_tx) ? (0x4) : (0)) | 0x1);
}

void lpGBT::setup_eclk(int ieclk, int rate, bool polarity, int strength) {
  if (ieclk < 0 || ieclk > 7) return;
  static constexpr uint8_t MAP_ECLK[8] = {28, 6, 4, 1, 19, 21, 27, 25};

  uint8_t which_clk = MAP_ECLK[ieclk];
  uint8_t ctl = 0;
  switch (rate) {
    case (40):
      ctl = 1;
      break;
    case (80):
      ctl = 2;
      break;
    case (160):
      ctl = 3;
      break;
    case (320):
      ctl = 4;
      break;
    case (640):
      ctl = 5;
      break;
    case (1280):
      ctl = 6;
      break;
    default:
      break;
  }
  ctl |= (strength & 0x7) << 3;
  if (!polarity) ctl |= 0x40;

  write(REG_ECLK_BASE + which_clk * 2, ctl);
  // currently no ability to mess with pre-emphasis
}

  void lpGBT::setup_i2c(int ibus, int speed_khz, bool scl_drive, bool strong_scl, bool strong_sda, bool pull_up_scl, bool pull_up_sda) {
    if (ibus<0 || ibus>2) return;

    uint8_t val;
    val=0;
    if (pull_up_scl) val|=0x40;
    if (pull_up_sda) val|=0x10;
    if (strong_scl) val|=0x20;
    if (strong_sda) val|=0x08;
    write(REG_I2CM0CONFIG+ibus*7,val);

    i2c_[ibus].ctl_reg=0;
    if (scl_drive) i2c_[ibus].ctl_reg|=0x80;
    if (speed_khz>125 && speed_khz<300) i2c_[ibus].ctl_reg|=0x01;
    if (speed_khz>300 && speed_khz<500) i2c_[ibus].ctl_reg|=0x02;
    if (speed_khz>500 && speed_khz<2000) i2c_[ibus].ctl_reg|=0x03;
    
  }

static constexpr uint8_t CMD_I2C_WRITE_CR = 0;
static constexpr uint8_t CMD_I2C_1BYTE_WRITE = 2;
static constexpr uint8_t CMD_I2C_1BYTE_READ = 3;
static constexpr uint8_t CMD_I2C_W_MULTI_4BYTE0 = 8;
static constexpr uint8_t CMD_I2C_W_MULTI_4BYTE1 = 9;
static constexpr uint8_t CMD_I2C_W_MULTI_4BYTE2 = 10;
static constexpr uint8_t CMD_I2C_W_MULTI_4BYTE3 = 11;
static constexpr uint8_t CMD_I2C_WRITE_MULTI = 0xC;
static constexpr uint8_t CMD_I2C_READ_MULTI = 0xD;
  
  void lpGBT::start_i2c_read(int ibus, uint8_t i2c_addr, int len) {
    if (ibus<0 || ibus>2 || len<0 || len>16) return;
    i2c_[ibus].read_len=len;
    write(REG_I2CM0ADDRESS+ibus*REG_I2C_WSTRIDE,i2c_addr);
    if (len==1) {
      write(REG_I2CM0DATA0+ibus*REG_I2C_WSTRIDE,i2c_[ibus].ctl_reg);
      write(REG_I2CM0CMD+ibus*REG_I2C_WSTRIDE,CMD_I2C_WRITE_CR);
      write(REG_I2CM0CMD+ibus*REG_I2C_WSTRIDE,CMD_I2C_1BYTE_READ);      
    } else {
      write(REG_I2CM0DATA0+ibus*REG_I2C_WSTRIDE,i2c_[ibus].ctl_reg|(len<<2));
      write(REG_I2CM0CMD+ibus*REG_I2C_WSTRIDE,CMD_I2C_WRITE_CR);
      write(REG_I2CM0CMD+ibus*REG_I2C_WSTRIDE,CMD_I2C_READ_MULTI);            
    }
  }

  void lpGBT::i2c_write(int ibus, uint8_t i2c_addr, uint8_t value) {
    if (ibus<0 || ibus>2) return;
    write(REG_I2CM0ADDRESS+ibus*REG_I2C_WSTRIDE,i2c_addr);
    write(REG_I2CM0DATA0+ibus*REG_I2C_WSTRIDE,i2c_[ibus].ctl_reg);
    write(REG_I2CM0CMD+ibus*REG_I2C_WSTRIDE,CMD_I2C_WRITE_CR);
    write(REG_I2CM0DATA0+ibus*REG_I2C_WSTRIDE,value);
    write(REG_I2CM0CMD+ibus*REG_I2C_WSTRIDE,CMD_I2C_1BYTE_WRITE);      
  }
  
  void lpGBT::i2c_write(int ibus, uint8_t i2c_addr, const std::vector<uint8_t>& values) {
    if (ibus<0 || ibus>2 || values.size()>16) return;
    write(REG_I2CM0ADDRESS+ibus*REG_I2C_WSTRIDE,i2c_addr);
    write(REG_I2CM0DATA0+ibus*REG_I2C_WSTRIDE,i2c_[ibus].ctl_reg|(values.size()<<2));
    write(REG_I2CM0CMD+ibus*REG_I2C_WSTRIDE,CMD_I2C_WRITE_CR);
    // copying all the data into the core...
    for (size_t i=0; i<values.size(); i++) {
      write(REG_I2CM0DATA0+(i%4)+ibus*REG_I2C_WSTRIDE,values[i]);
      // every 4 bytes or on the last byte
      if ((i%4)==3 || (i+1)==values.size())
	write(REG_I2CM0CMD+ibus*REG_I2C_WSTRIDE,CMD_I2C_W_MULTI_4BYTE0+(i/4));
    }
    // launch the write
    write(REG_I2CM0CMD+ibus*REG_I2C_WSTRIDE,CMD_I2C_WRITE_MULTI);      
  }

  bool lpGBT::i2c_transaction_check(int ibus, bool wait) {
    static constexpr uint8_t NOCLK   = 0x80;
    static constexpr uint8_t NOACK   = 0x40;
    static constexpr uint8_t LEVELE  = 0x08;
    static constexpr uint8_t SUCCESS = 0x04;
    
    if (ibus<0 || ibus>2) return false;
    do {
      uint8_t val=read(REG_I2CM0STATUS+ibus*REG_I2C_RSTRIDE);
      if (val&NOCLK) {
	PFEXCEPTION_RAISE("I2CErrorNoCLK", "No clock on I2C controller");
      }
      if (val&NOACK) {
	PFEXCEPTION_RAISE("I2CErrorNoACK", "No acknowledge from I2C target");
      }
      if (val&LEVELE) {
	PFEXCEPTION_RAISE("I2CErrorSDALow", "SDA Line Low on Start");
      }
      if (val&SUCCESS) {
	return true;
      }
      usleep(100); // 
    } while (wait);
    return false;      
  }
  
  std::vector<uint8_t> lpGBT::i2c_read_data(int ibus) {
    std::vector<uint8_t> retval;
    if (ibus<0 || ibus>2) return retval;
    if (i2c_[ibus].read_len==1) {
      retval.push_back(read(REG_I2CM0READBYTE+ibus*REG_I2C_RSTRIDE));
    } else {
      // super-weird -- it's stored in backwards order...
      retval=read(REG_I2CM0READ15+1-i2c_[ibus].read_len+ibus*REG_I2C_RSTRIDE,i2c_[ibus].read_len);
      std::reverse(retval.begin(),retval.end());
    }
    return retval;
  }

  
int lpGBT::status() { return read(REG_POWERUP_STATUS); }
std::string lpGBT::status_name(int pusm) {
  static const char* states[] = {"ARESET",
                                 "RESET1",
                                 "WAIT_VDD_STABLE",
                                 "WAIT_VDD_HIGHER_THAN_0V90",
                                 "STATE_COPY_FUSES",
                                 "STATE_CALCULATE_CHECKSUM",
                                 "COPY_ROM",
                                 "PAUSE_FOR_PLL_CONFIG",
                                 "WAIT_POWER_GOOD",
                                 "RESET_PLL",
                                 "WAIT_PLL_LOCK",
                                 "INIT_SCRAM",
                                 "RESETOUT",
                                 "I2C_TRANS",
                                 "PAUSE_FOR_DLL_CONFIG",
                                 "RESET_DLLS",
                                 "WAIT_DLL_LOCK",
                                 "RESET_LOGIC_USING_DLL",
                                 "WAIT_CHNS_LOCKED",
                                 "READY"};
  return states[pusm];
}

}  // namespace pflib
