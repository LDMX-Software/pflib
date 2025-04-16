#include "pflib/lpgbt/lpGBT_ConfigTransport_I2C.h"
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#ifndef I2C_SLAVE
#define I2C_SLAVE 0x0703
#endif 
namespace pflib {

lpGBT_ConfigTransport_I2C::lpGBT_ConfigTransport_I2C(uint8_t i2c_addr, const std::string& bus_dev) {
  handle_=open(bus_dev.c_str(), O_RDWR);
  ioctl(handle_,I2C_SLAVE,i2c_addr);
  if (handle_ < 0) {
    char msg[1000];
    snprintf(msg,1000,"Error %s (%d) opening I2C device file",strerror(errno),errno,bus_dev.c_str());
    PFEXCEPTION_RAISE("I2CException",msg);
  }
}
lpGBT_ConfigTransport_I2C::~lpGBT_ConfigTransport_I2C() {
  if (handle_>0) close(handle_);
}

void lpGBT_ConfigTransport_I2C::write_raw(uint8_t a) {
  if (write(handle_,&a,1)<0) {
    printf("Error on raw write %s\n",strerror(errno));
  }
}
void lpGBT_ConfigTransport_I2C::write_raw(uint8_t a, uint8_t b) {
  uint8_t buf[2]={a,b};
  if (write(handle_,buf,2)<0) {
    printf("Error on raw write %s\n",strerror(errno));
  }
}
void lpGBT_ConfigTransport_I2C::write_raw(uint8_t a, uint8_t b, uint8_t c) {
  uint8_t buf[3]={a,b,c};
  if (write(handle_,buf,3)<0) {
    printf("Error on raw write %s\n",strerror(errno));
  }
}
void lpGBT_ConfigTransport_I2C::write_raw(const std::vector<uint8_t>& a) {
  write(handle_,&(a[0]),a.size());
}

uint8_t lpGBT_ConfigTransport_I2C::read_raw() {
    uint8_t buf;
    if (read(handle_,&buf,1)<0) {
      printf("Error on raw read %s\n",strerror(errno));
    }
    return buf;
}

std::vector<uint8_t> lpGBT_ConfigTransport_I2C::read_raw(int n) {
  uint8_t buf[120];
  read(handle_,buf,n);
  std::vector<uint8_t> b;
  for (int i=0; i<n; i++) b.push_back(buf[i]);
  return b;
}

uint8_t lpGBT_ConfigTransport_I2C::read_reg(uint16_t reg) {
  uint8_t data[2]={uint8_t(reg&0xFF),uint8_t((reg>>8)&0xFF)};
  // write the register address, then read
  if (write(handle_,data,2)<0 || read(handle_,data,1)<0) {
    char msg[100];
    snprintf(msg,100,"Read from lpGBT register 0x%03x failed",reg);
    PFEXCEPTION_RAISE("I2CException",msg);
  }
  return data[0];
}

void lpGBT_ConfigTransport_I2C::write_reg(uint16_t reg, uint8_t val) {
  uint8_t data[3]={uint8_t(reg&0xFF),uint8_t((reg>>8)&0xFF),val};
  if (write(handle_,data,3)<0) {
    char msg[100];
    snprintf(msg,100,"Write 0x%02x to lpGBT register 0x%03x failed",val,reg);
    PFEXCEPTION_RAISE("I2CException",msg);
  }
}

void lpGBT_ConfigTransport_I2C::write_regs(uint16_t reg, const std::vector<uint8_t>& value) {
  uint8_t buffer[10];
  size_t ptr=0;
  int n=0;
  while (ptr<value.size()) {
    buffer[0]=uint8_t((reg+ptr)&0xFF);
    buffer[1]=uint8_t(((reg+ptr)>>8)&0xFF);
    n=0;
    for (size_t i=0; i<8 && ptr+i<value.size(); i++) { // limit to eight bytes
      buffer[2+i]=value[ptr+i];
      n++;
    }
    if (write(handle_,buffer,n+2)<0) {
      char msg[100];
      snprintf(msg,100,"Write to lpGBT register 0x%03x failed",reg);
      PFEXCEPTION_RAISE("I2CException",msg);
    }
    ptr+=n;
  }
}


std::vector<uint8_t> lpGBT_ConfigTransport_I2C::read_regs(uint16_t reg, int n) {
  std::vector<uint8_t> retval;
  uint8_t buffer[10];
  int ptr=0;
  int ni=0;
  while (int(ptr)<n) {
    buffer[0]=uint8_t((reg+ptr)&0xFF);
    buffer[1]=uint8_t(((reg+ptr)>>8)&0xFF);
    ni=(n>ptr+8)?(8):(n-ptr);

    if (write(handle_,buffer,2)<0 || read(handle_,buffer,ni)<0) {
      char msg[100];
      snprintf(msg,100,"Read from lpGBT register 0x%03x failed",reg);
      PFEXCEPTION_RAISE("I2CException",msg);
    }

    for (int i=0; i<ni; i++)
      retval.push_back(buffer[i]);
    ptr+=ni;
  }
  return retval;
}


}
