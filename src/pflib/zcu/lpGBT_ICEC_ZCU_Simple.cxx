#include "pflib/zcu/lpGBT_ICEC_ZCU_Simple.h"
#include "pflib/Exception.h"

namespace pflib {
namespace zcu {

static const int OFFSET_IC = 64;
static const int OFFSET_EC = 66;
static const uint32_t REG_ADDR_TX_DATA  =  0;
static const uint32_t MASK_TX_DATA      = 0x000000FF;
static const uint32_t MASK_I2C_ADDR     = 0x0000FF00;
static const uint32_t MASK_REG_ADDR     = 0xFFFF0000;
static const uint32_t REG_CTL_RESET_N_READ  =  1;
static const uint32_t MASK_N_READ       = 0x000000FF;
static const uint32_t MASK_RESET_RX     = 0x01000000;
static const uint32_t MASK_RESET_TX     = 0x02000000;
static const uint32_t MASK_START_WRITE  = 0x04000000;
static const uint32_t MASK_START_READ   = 0x08000000;
static const uint32_t MASK_TX_FIFO_LOAD = 0x10000000;
static const uint32_t MASK_RX_FIFO_ADV  = 0x20000000;
static const uint32_t MASK_RESET_ALL    = 0x40000000;
static const uint32_t REG_STATUS_READ   = 4;
static const uint32_t MASK_RX_DATA      = 0x000000FF;
static const uint32_t MASK_RX_EMPTY     = 0x00000100;
static const uint32_t MASK_TX_EMPTY     = 0x00000200;

lpGBT_ICEC_Simple::lpGBT_ICEC_Simple(const std::string& target, bool isEC, uint8_t lpgbt_i2c_addr) : offset_{isEC?(OFFSET_EC):(OFFSET_IC)}, lpgbt_i2c_addr_{lpgbt_i2c_addr}, transport_(target) {
  int reg=REG_CTL_RESET_N_READ+offset_;
  /*
  transport_.write(reg,MASK_RESET_RX|MASK_RESET_TX);
  transport_.write(reg,0);
  */
}

uint8_t lpGBT_ICEC_Simple::read_reg(uint16_t reg) {
  std::vector<uint8_t> retval=read_regs(reg,1);
  return retval[0];
}

std::vector<uint8_t> lpGBT_ICEC_Simple::read_regs(uint16_t reg, int n) {
  std::vector<uint8_t> retval;
  int wc=0;

  if (n>8) {
    retval=read_regs(reg,8);
    std::vector<uint8_t> later=read_regs(reg+8,n-8);
    retval.insert(retval.end(),later.begin(),later.end());
    return retval;
  }
  
  uint32_t val;
  // set addresses
  val = (uint32_t(reg)<<16) | (uint32_t(lpgbt_i2c_addr_)<<8);
  transport_.write(offset_+REG_ADDR_TX_DATA,val);
  // set length and start
  val = n|MASK_START_READ;
  transport_.write(offset_+REG_CTL_RESET_N_READ,val);
  // wait for done...
  int timeout=1000;
  for (val=transport_.read(offset_+REG_STATUS_READ); (val&MASK_RX_EMPTY); val=transport_.read(offset_+REG_STATUS_READ)) {
    usleep(1);
    timeout--;
    if (timeout==0) {
      PFEXCEPTION_RAISE("ICEC_Timeout","Read register timeout");
    }
  }
  wc=0;
  while (!(val&MASK_RX_EMPTY)) {
    uint8_t abyte=uint8_t(val&MASK_RX_DATA);
    transport_.write(offset_+REG_CTL_RESET_N_READ,MASK_RX_FIFO_ADV);
    if (wc>=6 && int(retval.size())<n) retval.push_back(abyte);
    wc++;
    val=transport_.read(offset_+REG_STATUS_READ);
  }
  
  return retval;
}

void lpGBT_ICEC_Simple::write_reg(uint16_t reg, uint8_t value) {
  std::vector<uint8_t> vv(1,value);
  write_regs(reg,vv);
}

void lpGBT_ICEC_Simple::write_regs(uint16_t reg, const std::vector<uint8_t>& value) {
  size_t wc=0;

  uint32_t baseval;
  // set addresses
  baseval = (uint32_t(reg)<<16) | (uint32_t(lpgbt_i2c_addr_)<<8);

  int ic=0;
  while (wc<value.size()) {
    transport_.write(offset_+REG_ADDR_TX_DATA,baseval|uint32_t(value[wc]));
    wc++; ic++;
    transport_.write(offset_+REG_CTL_RESET_N_READ,MASK_TX_FIFO_LOAD);
    if (ic==8 || wc==value.size()) {
      transport_.write(offset_+REG_CTL_RESET_N_READ,MASK_START_WRITE);
      // wait for tx to be done
      int timeout=1000;
      for (uint32_t val=transport_.read(offset_+REG_STATUS_READ); (val&MASK_TX_EMPTY); val=transport_.read(offset_+REG_STATUS_READ)) {
        usleep(1);
        timeout--;
        if (timeout==0) {
          PFEXCEPTION_RAISE("ICEC_Timeout","Write register timeout");
        }
      }
      ic=0;
    }
  }
}


}
}
