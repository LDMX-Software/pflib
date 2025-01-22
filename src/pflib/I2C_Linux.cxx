#include "pflib/I2C_Linux.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>

namespace pflib {

I2C_Linux::I2C_Linux(const std::string& device) {
  dev_=device;
  handle_=open(device.c_str(),O_RDWR);
  if (handle_<0) {
    std::string msg="Unable to open "+device;
    PFEXCEPTION_RAISE("DeviceFileAccessError",msg);
  }
}

I2C_Linux::~I2C_Linux() {
  if (handle_>0)
    close(handle_);
  handle_=0;
}

void I2C_Linux::set_bus_speed(int speed) {
  // can't actually do this!
}

int I2C_Linux::get_bus_speed() {
  return 100; // only as set in the firmware...
}

void I2C_Linux::write_byte(uint8_t i2c_dev_addr, uint8_t data) {
  ioctl(handle_,0x0703, i2c_dev_addr);

  int rv;
  int tries=4;
  do {
    rv=write(handle_,&data,1);

    if (rv>=0) {
       printf("Wrote %x, read %x\n",data,read_byte(i2c_dev_addr));
       return;
    }

    if (tries==0) {
      char message[120];
      snprintf(message,120,"Error %d writing 0x%02x to i2c target 0x%02x",errno,data,i2c_dev_addr);
      PFEXCEPTION_RAISE("I2CError",message);
    }
    tries--;
  } while (rv<0);

}

uint8_t I2C_Linux::read_byte(uint8_t i2c_dev_addr) {
  ioctl(handle_,0x0703, i2c_dev_addr);
  uint8_t buffer[8];
  int rv=read(handle_,buffer,1);

  if (rv<0) {
    char message[120];
    snprintf(message,120,"Error %d reading from i2c target 0x%02x",errno,i2c_dev_addr);
    PFEXCEPTION_RAISE("I2CError",message);
  }

  return buffer[0];
}

  
std::vector<uint8_t> I2C_Linux::general_write_read(uint8_t i2c_dev_addr, const std::vector<uint8_t>& wdata, int nread) {
  ioctl(handle_,0x0703, i2c_dev_addr);

  if (wdata.size()>0) {
    int ret=write(handle_,&(wdata[0]),wdata.size());

    if (ret<0) {
      char message[120];
      snprintf(message,120,"Error %d writing to i2c target 0x%02x",errno,i2c_dev_addr);
      PFEXCEPTION_RAISE("I2CError",message);
    }
  }
    
  std::vector<uint8_t> rv(nread,0);

  if (nread>0) {
    int ret=read(handle_,&rv[0],nread);

    if (ret<0) {
      char message[120];
      snprintf(message,120,"Error %d reading from i2c target 0x%02x",errno,i2c_dev_addr);
      PFEXCEPTION_RAISE("I2CError",message);
    }
  }
  
  return rv;
}

}
