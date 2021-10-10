#include "pflib/I2C.h"
#include <stdio.h>
#include <unistd.h>

namespace pflib {

static const uint32_t REG_BUS_SELECT           = 2;
static const uint32_t MASK_BUS_SELECT          = 0xFFFF;
static const uint32_t REG_BUS_AVAILABLE        = 1;
static const uint32_t MASK_BUS_AVAILABLE       = 0xFFFF;

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

}
