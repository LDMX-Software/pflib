#include "pflib/I2C.h"
#include <stdio.h>
#include <unistd.h>

namespace pflib {

static const uint32_t REG_BUS_SELECT           = 2;
static const uint32_t MASK_BUS_SELECT          = 0xFFFF;
static const uint32_t REG_BUS_AVAILABLE        = 1;
static const uint32_t MASK_BACKPLANE_HACK      = 0x30000;
static const uint32_t MASK_BUS_AVAILABLE       = 0x0FFFF;

static const uint32_t REG_TRANSACTOR           = 5;
static const uint32_t MASK_TRANSACTOR_READ     = 0x1;
static const uint32_t MASK_TRANSACTOR_I2CADDR  = 0xFE;
static const uint32_t SHFT_TRANSACTOR_I2CADDR  = 1;
static const uint32_t MASK_TRANSACTOR_ENABLE   = 0x80000000;
static const uint32_t MASK_TRANSACTOR_START    = 0x40000000;
static const uint32_t MASK_TRANSACTOR_BUSY     = 0x04000000;
static const uint32_t MASK_TRANSACTOR_DONE     = 0x02000000;
static const uint32_t MASK_TRANSACTOR_ERROR    = 0x01000000;
static const uint32_t MASK_TRANSACTOR_TX       = 0x0000FF00;
static const uint32_t SHFT_TRANSACTOR_TX       = 8;
static const uint32_t MASK_TRANSACTOR_RX       = 0x00FF0000;
static const uint32_t SHFT_TRANSACTOR_RX       = 16;

static const uint32_t REG_PRESCALE0            = 8;
static const uint32_t REG_PRESCALE1            = 9;


void I2C::set_active_bus(int ibus) {
  int maxbus=get_bus_count();
  if (ibus>=maxbus) {
    char message[128];
    snprintf(message,128,"Requested bus %d but only %d busses are available",ibus,maxbus);
    PFEXCEPTION_RAISE("I2C_BUS_OUT_OF_RANGE",message);
  }
  if (ibus<0) ibus=0x1000; // disable

  wb_rmw(REG_BUS_SELECT,ibus,MASK_BUS_SELECT);
}

int I2C::get_active_bus() {
  return (wb_read(REG_BUS_SELECT))&MASK_BUS_SELECT;
}

int I2C::get_bus_count() {
  return (wb_read(REG_BUS_AVAILABLE))&MASK_BUS_AVAILABLE;
}

void I2C::set_bus_speed(int speed) {
  const int clockspeed=38000; // nominal clock speed
  uint32_t prescaler=clockspeed/(speed*5);
  wb_write(REG_PRESCALE0,prescaler&0xFF);
  wb_write(REG_PRESCALE1,(prescaler>>8)&0xFF);
  //
  usec_per_byte_=8*1000/speed;
}

int I2C::get_bus_speed() {
  const int clockspeed=38000; // nominal clock speed
  uint32_t prescaler=(wb_read(REG_PRESCALE1)<<8)+wb_read(REG_PRESCALE0);
  int speed=clockspeed/(prescaler*5);
  return speed;
}

  void I2C::backplane_hack(bool enable) {
    if (enable) wb_rmw(REG_BUS_SELECT,MASK_BACKPLANE_HACK,MASK_BACKPLANE_HACK);
    else wb_rmw(REG_BUS_SELECT,0,MASK_BACKPLANE_HACK);
  }

void I2C::write_byte(uint8_t i2c_dev_addr, uint8_t data) {
  uint32_t val=MASK_TRANSACTOR_ENABLE|MASK_TRANSACTOR_START;
  val|=(i2c_dev_addr<<SHFT_TRANSACTOR_I2CADDR)&MASK_TRANSACTOR_I2CADDR;
  val|=(data<<SHFT_TRANSACTOR_TX);
  // launch the transactor
  wb_write(REG_TRANSACTOR,val);
  do {
    usleep(std::max(1,usec_per_byte_/2));
    val=wb_read(REG_TRANSACTOR);
  } while (val&MASK_TRANSACTOR_BUSY);
  if (val&MASK_TRANSACTOR_ERROR) {
    char message[120];
    snprintf(message,120,"Error writing 0x%02x to i2c target 0x%02x",data,i2c_dev_addr);
    PFEXCEPTION_RAISE("I2CError",message);
  }
  wb_write(REG_TRANSACTOR,0); // disable
}

  static const int I2C_PRE0 = 0|8;
  static const int I2C_PRE1 = 1|8;
  static const int I2C_CTR = 2|8;
  static const int I2C_TXR = 3|8;
  static const int I2C_RXR = 3|8;
  static const int I2C_CR  = 4|8;
  static const int I2C_SR  = 4|8;
  
  static const int I2C_STA = 0x80;
  static const int I2C_STO = 0x40;
  static const int I2C_RD  = 0x20;
  static const int I2C_WR  = 0x10;
  static const int I2C_NACK= 0x08;
  static const int I2C_IACK= 0x01;

  bool I2C::i2c_wait(bool nack_ok) {
    uint32_t val;
    bool done=false;
    int tries=100;
    do {
      val=wb_read(I2C_SR);
      if (!(val & 0x2)) done=true;
      tries--;
      if (tries<5) {
	printf("Bad scene! %x\n",val);
      }
    } while (!done && tries>0);
    if (!done || ((val & 0x80)!=0 && !nack_ok)) {
      char message[128];
      sprintf(message,"I2C WAIT FAILED -> %02x %d\n",val,done);
      wb_write(I2C_CR, I2C_STO); // send a stop to avoid locking up the bus
      wb_write(I2C_CTR, 0x00); // close the core
      PFEXCEPTION_RAISE("I2CError",message);
      return false; //irrelevant...
    }
    return true;
  }
  
  uint8_t I2C::read_byte(uint8_t i2c_dev_addr) {
    uint32_t val=MASK_TRANSACTOR_ENABLE|MASK_TRANSACTOR_START|MASK_TRANSACTOR_READ;
    val|=(i2c_dev_addr<<SHFT_TRANSACTOR_I2CADDR)&MASK_TRANSACTOR_I2CADDR;
    // launch the transactor
    wb_write(REG_TRANSACTOR,val);
    do {
      usleep(std::max(1,usec_per_byte_/2));
      val=wb_read(REG_TRANSACTOR);
    } while (val&MASK_TRANSACTOR_BUSY);
    if (val&MASK_TRANSACTOR_ERROR) {
      char message[120];
      snprintf(message,120,"Error reading from i2c target 0x%02x",i2c_dev_addr);
      PFEXCEPTION_RAISE("I2CError",message);
    }
    wb_write(REG_TRANSACTOR,0); // disable
    return (val&MASK_TRANSACTOR_RX)>>SHFT_TRANSACTOR_RX;
  }
  
  std::vector<uint8_t> I2C::general_write_read(uint8_t i2c_dev_addr, const std::vector<uint8_t>& wdata, int nread) {
    std::vector<uint8_t> rv;
    // enable the core
    wb_write(I2C_CTR,0x80);

    // first, the write cycles
    if (wdata.size()>0) {
      // set the address
      wb_write(I2C_TXR, (i2c_dev_addr<<1));
      // launch the write
      wb_write(I2C_CR, I2C_STA | I2C_WR);
      // wait for completion
      if (!i2c_wait()) return rv; // automatically shuts down the core

      for (size_t i=0; i<wdata.size(); i++) {
	wb_write(I2C_TXR, wdata[i]);
	if (i+1==wdata.size() && nread==0) wb_write(I2C_CR, I2C_STO | I2C_WR);
	else wb_write(I2C_CR, I2C_WR);

	if (!i2c_wait(i+1==wdata.size())) return rv; // automatically shuts down the core
      }
    }
    // now, if requested, the read cycles...
    if (nread>0) {
      // set the address
      wb_write(I2C_TXR, (i2c_dev_addr<<1)|1);
      // launch the write
      wb_write(I2C_CR, I2C_STA | I2C_WR);
      // wait for completion
      if (!i2c_wait()) return rv; // automatically shuts down the core
      
      for (int i=0; i<nread; i++) {
	if (i+1==nread) wb_write(I2C_CR, I2C_STO | I2C_RD | I2C_NACK);
	else wb_write(I2C_CR, I2C_RD);

	if (!i2c_wait(i+1==nread)) return rv; // automatically shuts down the core

	uint32_t val=wb_read(I2C_RXR);
	rv.push_back(uint8_t(val&0xFF));

      }
    }

    wb_write(I2C_CTR, 0); // shut down the core
    return rv;
  }

}
