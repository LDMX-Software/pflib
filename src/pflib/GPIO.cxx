#include "pflib/GPIO.h"

namespace pflib {

static const uint32_t REG_PORTCOUNTS           = 1;
static const uint32_t MASK_PORTCOUNTS_INPUT    = 0x0000ffffu;
static const uint32_t SHFT_PORTCOUNTS_INPUT    = 0;
static const uint32_t MASK_PORTCOUNTS_OUTPUT   = 0xffff0000u;
static const uint32_t SHFT_PORTCOUNTS_OUTPUT   = 16;
static const uint32_t REG_INPUTS_BASE          = 8;
static const uint32_t REG_OUTPUTS_BASE         = 16;

static const int BITS_PER_REG                  = 32;

int GPIO::getGPOcount() {
  if (ngpo_>=0) return ngpo_;
  ngpo_=(wb_read(REG_PORTCOUNTS)&MASK_PORTCOUNTS_OUTPUT)>>SHFT_PORTCOUNTS_OUTPUT;
  return ngpo_;
}

int GPIO::getGPIcount() {
  if (ngpi_>=0) return ngpi_;
  ngpi_=(wb_read(REG_PORTCOUNTS)&MASK_PORTCOUNTS_INPUT)>>SHFT_PORTCOUNTS_INPUT;
  return ngpi_;
}

bool GPIO::getGPI(int ibit) {
  if (ibit>=ngpi_) {
     char message[120];
     snprintf(message,120,"GPI bit %d is out of range with maxGPI %d",ibit,ngpi_);
     PFEXCEPTION_RAISE("GPIOError",message);
  }
  int ireg=ibit/BITS_PER_REG;
  int lbit=ibit%BITS_PER_REG;
  return ((wb_read(REG_INPUTS_BASE+ireg)>>lbit)&0x1)==0x1;
}

std::vector<bool> GPIO::getGPI() {
  if (ngpi_<0) getGPIcount();
  std::vector<bool> retval(ngpi_,false);
  if (ngpi_==0) return retval;
  for (int ireg=0; ireg<=(ngpi_-1)/BITS_PER_REG; ireg++) {
    uint32_t val=wb_read(REG_INPUTS_BASE+ireg);
    for (int ibit=0; ibit<BITS_PER_REG && (ibit+ireg*BITS_PER_REG)<ngpi_; ibit++) {
      bool bitval=((val>>ibit)&0x1)==0x1;
      retval[ibit+ireg*BITS_PER_REG]=bitval;
    }
  }
  return retval;
}

void GPIO::setGPO(int ibit, bool toTrue) {
  if (ibit>=ngpo_) {
     char message[120];
     snprintf(message,120,"GPO bit %d is out of range with maxGPO=%d",ibit,ngpo_);
     PFEXCEPTION_RAISE("GPIOError",message);
  }
  int ireg=ibit/BITS_PER_REG;
  int lbit=ibit%BITS_PER_REG;
  uint32_t val=wb_read(REG_OUTPUTS_BASE+ireg);
  // set the bit and then invert it if needed
  val|=(1<<lbit);
  if (!toTrue) val^=(1<<lbit);
  wb_write(REG_OUTPUTS_BASE+ireg,val);
}

void GPIO::setGPO(const std::vector<bool>& bits) {
  if (int(bits.size())!=ngpo_) {
     char message[120];
     snprintf(message,120,"GPO bit array size(%d) != nGPO(%d)",int(bits.size()),ngpo_);
     PFEXCEPTION_RAISE("GPIOError",message);
  }
  for (int ireg=0; ireg<=(ngpo_-1)/BITS_PER_REG; ireg++) {
    uint32_t val=0;
    for (int ibit=0; ibit<BITS_PER_REG && (ibit+ireg*BITS_PER_REG)<ngpo_; ibit++) {
      if (bits[ibit+ireg*BITS_PER_REG]) val|=(1<<ibit);      
    }
    wb_write(REG_OUTPUTS_BASE+ireg,val);
  }
}

std::vector<bool> GPIO::getGPO() {
  if (ngpo_<0) getGPOcount();
  std::vector<bool> retval(ngpo_,false);
  if (ngpo_==0) return retval;
  for (int ireg=0; ireg<=(ngpo_-1)/BITS_PER_REG; ireg++) {
    uint32_t val=wb_read(REG_OUTPUTS_BASE+ireg);
    for (int ibit=0; ibit<BITS_PER_REG && (ibit+ireg*BITS_PER_REG)<ngpo_; ibit++) {
      bool bitval=((val>>ibit)&0x1)==0x1;
      retval[ibit+ireg*BITS_PER_REG]=bitval;
    }
  }
  return retval;
}


}
