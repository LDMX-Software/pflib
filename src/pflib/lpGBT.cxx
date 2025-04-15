#include "pflib/lpGBT.h"
#include <unistd.h>

namespace pflib {

std::vector<uint8_t> lpGBT_ConfigTransport::read_regs(uint16_t reg, int n) {
  std::vector<uint8_t> v;
  for (int i=0; i<n; i++)
    v.push_back(read_reg(reg+i));
  return v;
}
void lpGBT_ConfigTransport::write_regs(uint16_t reg, const std::vector<uint8_t>& value)  {
  for (size_t i=0; i<value.size(); i++) 
    write_reg(i+reg,value[i]);
}


lpGBT::lpGBT(lpGBT_ConfigTransport& transport) : tport_{transport} {
}

void lpGBT::write(const RegisterValueVector& regvalues) {
  std::vector<uint8_t> tosend;
  static const uint16_t UNDEF_ADDR = 0xFFFF;
  uint16_t base_addr=UNDEF_ADDR;

  // slightly complex, to allow batching of writes to optimize I/O usage
  
  for (size_t ptr=0; ptr<regvalues.size(); ptr++) {
    if (ptr+1<regvalues.size() && regvalues[ptr+1].first==regvalues[ptr].first+1) { // we will start or continue to build a sequence...
      if (base_addr==UNDEF_ADDR) base_addr=regvalues[ptr].first; // start a new sequence
      tosend.push_back(regvalues[ptr].second);
    } else if (base_addr!=UNDEF_ADDR) { // this means the current value must be in sequence, even though the next one isn't
      tosend.push_back(regvalues[ptr].second);
      tport_.write_regs(base_addr,tosend);
      tosend.clear();
      base_addr=UNDEF_ADDR;
    } else { // we are a one-off
      tport_.write_reg(regvalues[ptr].first,regvalues[ptr].second);
    }      
  }

}

lpGBT::RegisterValueVector lpGBT::read(const std::vector<uint16_t>& registers) {
  RegisterValueVector retval;

  static const uint16_t UNDEF_ADDR = 0xFFFF;
  uint16_t base_addr=UNDEF_ADDR;
  int n=-1;

  // slightly complex, to allow batching of reads to optimize I/O usage
  
  for (size_t ptr=0; ptr<registers.size(); ptr++) {
    if (ptr+1<registers.size() && registers[ptr+1]==registers[ptr]+1) { // we will start or continue to build a sequence...
      if (base_addr==UNDEF_ADDR) {
        base_addr=registers[ptr]; // start a new sequence
        n=0;
      }
      n++;
    } else if (base_addr!=UNDEF_ADDR) { // this means the current register must be in sequence, even though the next one isn't
      n++;
      std::vector<uint8_t> values=tport_.read_regs(base_addr,n);
      for (int i=0; i<n; i++)
        retval.push_back(RegisterValue(base_addr+i,values[i]));
      base_addr=UNDEF_ADDR;
    } else { // we are a one-off
      retval.push_back(RegisterValue(registers[ptr],tport_.read_reg(registers[ptr])));
    }
  }
    return retval;
}

void lpGBT::bit_set(uint16_t reg, int ibit) {
  uint8_t cval=tport_.read_reg(reg);
  cval|=(1<<(ibit%8));
  tport_.write_reg(reg,cval);
}
void lpGBT::bit_clr(uint16_t reg, int ibit) {
  uint8_t cval=tport_.read_reg(reg);
  cval|=(1<<(ibit%8));
  cval^=(1<<(ibit%8));
  tport_.write_reg(reg,cval);
}

/// Register constants here are all correct for V1 and V2 lpGBTs
static constexpr uint16_t REG_PIODIRH         = 0x053;
static constexpr uint16_t REG_PIOPULLENAH     = 0x057;
static constexpr uint16_t REG_PIOUPDOWNH      = 0x059;
static constexpr uint16_t REG_PIODRIVEH       = 0x05b;

static constexpr uint16_t REG_PIOOUTH         = 0x055;
static constexpr uint16_t REG_PIOOUTL         = 0x056;
static constexpr uint16_t REG_PIOINH          = 0x1AF;
static constexpr uint16_t REG_PIOINL          = 0x1B0;


static constexpr uint16_t REG_VREFCNTR        = 0x01c;
static constexpr uint16_t REG_DAC_CONFIG_H    = 0x06a;
static constexpr uint16_t REG_CURDAC_VALUE    = 0x06c;
static constexpr uint16_t REG_CURDAC_CHN      = 0x06d;
static constexpr uint16_t REG_ADC_SELECT      = 0x121;
static constexpr uint16_t REG_ADCMON          = 0x122;
static constexpr uint16_t REG_ADC_CONFIG      = 0x123;
static constexpr uint16_t REG_ADC_STATUS_H    = 0x1ca;
static constexpr uint16_t REG_ADC_STATUS_L    = 0x1cb;


void lpGBT::gpio_set(int ibit, bool high) {
  if (ibit<0 || ibit>15) {
    char msg[100];
    snprintf(msg, 100, "GPIO bit %d is out of range 0:15", ibit);
    PFEXCEPTION_RAISE("OutOfRangeException",msg);
  }
  uint16_t reg=(ibit<8)?(REG_PIOOUTL):(REG_PIOOUTH);
  if (high) bit_set(reg,ibit%8);
  else bit_clr(reg,ibit%8);
}


bool lpGBT::gpio_get(int ibit) {
  if (ibit<0 || ibit>15) {
    char msg[100];
    snprintf(msg, 100, "GPIO bit %d is out of range 0:15", ibit);
    PFEXCEPTION_RAISE("OutOfRangeException",msg);
  }
  uint16_t reg=(ibit<8)?(REG_PIOINL):(REG_PIOINH);
  uint8_t value=tport_.read_reg(reg);
  return (value&(1<<(ibit&8)));
}

void lpGBT::gpio_set(uint16_t values) {
  std::vector<uint8_t> vals;
  vals.push_back(uint8_t(values>>8));
  vals.push_back(uint8_t(values&0xFF));
  tport_.write_regs(REG_PIOOUTH,vals);
}

uint16_t lpGBT::gpio_get() {
  std::vector<uint8_t> vals = tport_.read_regs(REG_PIOINH,2);
  return uint16_t(vals[1])|((uint16_t(vals[0]))<<8);
}

int lpGBT::gpio_cfg_get(int ibit) {
  int cfg=0;
  if (ibit<0 || ibit>15) {
    char msg[100];
    snprintf(msg, 100, "GPIO bit %d is out of range 0:15", ibit);
    PFEXCEPTION_RAISE("OutOfRangeException",msg);
  }
  int offset=(ibit<8)?(1):(0); // fixed relationship between H and L bytes
  if (read(REG_PIODIRH+offset)) cfg|=GPIO_IS_OUTPUT;
  if (read(REG_PIODRIVEH+offset)) cfg|=GPIO_IS_STRONG;
  if (read(REG_PIOPULLENAH+offset)) {
    if (read(REG_PIOUPDOWNH+offset)) cfg|=GPIO_IS_PULLUP;
    else cfg|=GPIO_IS_PULLDOWN;
  }
  return cfg;
}


void lpGBT::gpio_cfg_set(int ibit, int cfg) {
  if (ibit<0 || ibit>15) {
    char msg[100];
    snprintf(msg, 100, "GPIO bit %d is out of range 0:15", ibit);
    PFEXCEPTION_RAISE("OutOfRangeException",msg);
  }
  int offset=(ibit<8)?(1):(0); // fixed relationship between H and L bytes
  if (cfg & GPIO_IS_OUTPUT) bit_set(REG_PIODIRH+offset,ibit%8);
  else bit_clr(REG_PIODIRH+offset,ibit%8);
  if (cfg & GPIO_IS_STRONG) bit_set(REG_PIODRIVEH+offset,ibit%8);
  else bit_clr(REG_PIODRIVEH+offset,ibit%8);

  if ((cfg&0x6)==GPIO_IS_PULLUP) {
    bit_set(REG_PIOUPDOWNH+offset,ibit%8);
    bit_set(REG_PIOPULLENAH+offset,ibit%8);
  } else if ((cfg&0x6)==GPIO_IS_PULLDOWN) {
    bit_clr(REG_PIOUPDOWNH+offset,ibit%8);
    bit_set(REG_PIOPULLENAH+offset,ibit%8);
  } else {
    bit_clr(REG_PIOPULLENAH+offset,ibit%8);
  }
}

uint16_t lpGBT::adc_resistance_read(int ipos, int current, int gain) {
  if (ipos>7) {
    PFEXCEPTION_RAISE("ADCException","Selected channel out of range for resistance measurement");
  }
  static constexpr int CURDAC_ENABLE = 6; // bit 6
  
  bit_set(REG_DAC_CONFIG_H,CURDAC_ENABLE);
  tport_.write_reg(REG_CURDAC_VALUE, current & (0xff));
  tport_.write_reg(REG_CURDAC_CHN, (1 << ipos));

  uint16_t value=adc_read(ipos,15,gain);

  bit_clr(REG_DAC_CONFIG_H,CURDAC_ENABLE);
  
  return value;
}

uint16_t lpGBT::adc_read(int ipos, int ineg, int gain) {
  // bit definitions
  static constexpr uint8_t M_ADC_CONFIG_CONVERT = uint8_t(1)<<7;
  static constexpr uint8_t M_ADC_CONFIG_ENABLE  = uint8_t(1)<<2;
  static constexpr uint8_t M_ADC_STATUS_H_DONE  = uint8_t(1)<<6;
  static constexpr uint8_t M_VREF_ENABLE        = uint8_t(1)<<7;


  if (ipos>=10 && ipos<=13) { // must enable the ADCMON
    tport_.write_reg(REG_ADCMON, 0x1F);
  }
  // work out the gain value
  int gval=0;
  if (gain==8) gval=1;
  if (gain==16) gval=2;
  if (gain==32) gval=3;

  // set up the multiplexers
  tport_.write_reg(REG_ADC_SELECT,(ipos<<4)|(ineg));
  // enable the ADC and set the gain
  tport_.write_reg(REG_ADC_CONFIG,M_ADC_CONFIG_ENABLE | gval);
  // enable vref
  tport_.write_reg(REG_VREFCNTR, M_VREF_ENABLE);
  usleep(1000);
  // start conversion
  tport_.write_reg(REG_ADC_CONFIG,M_ADC_CONFIG_CONVERT | M_ADC_CONFIG_ENABLE | gval);

  // wait and check if done
  while (!(tport_.read_reg(REG_ADC_STATUS_H)&M_ADC_STATUS_H_DONE)) 
    usleep(1000);

  uint16_t adc_value = ((tport_.read_reg(REG_ADC_STATUS_H)&0x3)<<8)|tport_.read_reg(REG_ADC_STATUS_L);

  // shut things down
  tport_.write_reg(REG_ADC_CONFIG, M_ADC_CONFIG_ENABLE | gain); 

  if (ipos>=10 && ipos<=13)
    tport_.write_reg(REG_ADCMON,0);
  tport_.write_reg(REG_VREFCNTR,0);
  
  return adc_value;
}

}
