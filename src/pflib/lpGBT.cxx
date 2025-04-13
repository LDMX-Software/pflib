#include "pflib/lpGBT.h"

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

static const uint16_t REG_PIOOUTH         = 0x055;
static const uint16_t REG_PIOOUTL         = 0x056;
static const uint16_t REG_PIOINH          = 0x1AF;
static const uint16_t REG_PIOINL          = 0x1B0;


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

uint16_t lpGBT::gpio_set() {
  std::vector<uint8_t> vals = tport_.read_regs(REG_PIOINH,2);
  return uint16_t(vals[1])|((uint16_t(vals[0]))<<8);
}
  


}
