#include "pflib/GPIO.h"
#include <linux/gpio.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>
#include <cerrno>
#include "pflib/Exception.h"
#include <string.h>
#include <sys/ioctl.h>

namespace pflib {

class GPIO_HcalHGCROCZCU : public GPIO {
 public:
  GPIO_HcalHGCROCZCU() : GPIO(8,1) {
    gpiodev_=open("/dev/gpiochip4",O_RDWR);
    if (gpiodev_<0) {
      char msg[100];
      snprintf(msg,100,"Error opening /dev/gpiochip4 : %d", errno);
      PFEXCEPTION_RAISE("DeviceFileAccessError",msg);
    }   
  }

  virtual std::vector<bool> getGPI();

  virtual void setGPO(int ibit, bool toTrue=true);
  virtual void setGPO(const std::vector<bool>& bits);
  
  virtual std::vector<bool> getGPO();

 private:
  void setup();
  
  int gpiodev_;
};

static const char* myname="PFLIB_GPIO_HCALROCZCU";

void GPIO_HcalHGCROCZCU::setup() {
  gpio_v2_line_request req;

  // clear out the request memory space
  memset(&req,0,sizeof(req));
  
  // gpo request
  for (int i=0; i<getGPOcount(); i++) {
    req.offsets[i]=i;
  }
  strncpy(req.consumer,myname,GPIO_MAX_NAME_SIZE);
  req.num_lines=getGPOcount();
  req.config.flags=GPIO_V2_LINE_FLAG_OUTPUT;
  req.config.num_attrs=0;

  if (ioctl(gpiodev_,GPIO_V2_LINE_SET_CONFIG_IOCTL,&req)) {
    char msg[100];
    snprintf(msg,100,"Error in setting up GPIO outputs %d", errno);
    PFEXCEPTION_RAISE("DeviceFileAccessError",msg);
  }  
  
}

void GPIO_HcalHGCROCZCU::setGPO(int ibit, bool toTrue) {
  if (ibit<0 || ibit>=getGPOcount()) {
    PFEXCEPTION_RAISE("GPIOError","Requested bit out of range");
  }
  gpio_v2_line_values req;
  req.mask=1<<ibit;
  
  if (toTrue) req.bits=req.mask;
  else req.bits=0;

  if (ioctl(gpiodev_,GPIO_V2_LINE_SET_VALUES_IOCTL,&req)) {
    char msg[100];
    snprintf(msg,100,"Error in writing GPO %d : %d", ibit, errno);
    PFEXCEPTION_RAISE("DeviceFileAccessError",msg);
  }  

}
  
  
void GPIO_HcalHGCROCZCU::setGPO(const std::vector<bool>& bits) {
  if (int(bits.size())!=getGPOcount()) {
    PFEXCEPTION_RAISE("GPIOError","Requested bits out of range");
  }
}


std::vector<bool> GPIO_HcalHGCROCZCU::getGPO() {
  // GPOs are lines 0-7
  std::vector<bool> retval;
  gpio_v2_line_values req;
  req.mask=0xFF;
  if (ioctl(gpiodev_,GPIO_V2_LINE_GET_VALUES_IOCTL,&req)) {
    char msg[100];
    snprintf(msg,100,"Error in reading GPOs : %d", errno);
    PFEXCEPTION_RAISE("DeviceFileAccessError",msg);
  }  
  
  for (int i=0; i<getGPOcount(); i++)
    retval.push_back((req.bits&(1<<(i)))!=0);
  return retval;
}

std::vector<bool> GPIO_HcalHGCROCZCU::getGPI() {
  // GPIs are lines 8
  std::vector<bool> retval;
  gpio_v2_line_values req;
  req.mask=0x800;
  if (ioctl(gpiodev_,GPIO_V2_LINE_GET_VALUES_IOCTL,&req)) {
    char msg[100];
    snprintf(msg,100,"Error in reading GPIs : %d", errno);
    PFEXCEPTION_RAISE("DeviceFileAccessError",msg);
  }  
  
  for (int i=0; i<getGPIcount(); i++)
    retval.push_back((req.bits&(1<<(i+getGPOcount())))!=0);
  return retval;
}

GPIO* make_GPIO_HcalHGCROCZCU() { return new GPIO_HcalHGCROCZCU(); }
}
