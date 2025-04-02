#include <fcntl.h>
#include <linux/gpio.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cerrno>

#include "pflib/Exception.h"
#include "pflib/GPIO.h"

namespace pflib {

class GPIO_HcalHGCROCZCU : public GPIO {
 public:
  GPIO_HcalHGCROCZCU() : GPIO(8, 1) {
    gpiodev_ = open("/dev/gpiochip4", O_RDWR);
    if (gpiodev_ < 0) {
      char msg[100];
      snprintf(msg, 100, "Error opening /dev/gpiochip4 : %d", errno);
      PFEXCEPTION_RAISE("DeviceFileAccessError", msg);
    }
    setup();
  }
  ~GPIO_HcalHGCROCZCU() {
    if (gpiodev_ > 0) close(gpiodev_);
  }

  virtual std::vector<bool> getGPI();

  static constexpr const char* gpo_names[] = {
      "HGCROC_RSTB_I2C", "HGCROC_SOFT_RSTB", "HGCROC_HARD_RSTB", "UNUSED1",
      "HGCROC_ADDR0",    "HGCROC_ADDR1",     "SCL_PHASE",        "POWER_OFF"};

  virtual std::string getBitName(int ibit, bool isgpo) {
    if (isgpo)
      return gpo_names[ibit];
    else
      return "HGCROC_ERROR";
  }

  virtual void setGPO(int ibit, bool toTrue = true);
  virtual void setGPO(const std::vector<bool>& bits);

  virtual std::vector<bool> getGPO();

 private:
  void setup();

  int gpiodev_;
  int fd_gpo_;
};

static const char* myname = "PFLIB_GPIO_HCALROCZCU";

void GPIO_HcalHGCROCZCU::setup() {
  gpio_v2_line_request req;

  // clear out the request memory space
  memset(&req, 0, sizeof(req));

  // gpo request
  for (int i = 0; i < getGPOcount(); i++) {
    req.offsets[i] = i;
  }
  strncpy(req.consumer, myname, GPIO_MAX_NAME_SIZE);
  req.num_lines = getGPOcount();
  req.config.flags = GPIO_V2_LINE_FLAG_OUTPUT;
  req.config.num_attrs = 1;

  req.config.attrs[0].mask =
      0x7;  // must have the resets default disabled on startup
  req.config.attrs[0].attr.id = GPIO_V2_LINE_ATTR_ID_OUTPUT_VALUES;
  req.config.attrs[0].attr.values = 0x7;

  if (ioctl(gpiodev_, GPIO_V2_GET_LINE_IOCTL, &req)) {
    char msg[100];
    snprintf(msg, 100, "Error in setting up GPIO outputs %d", errno);
    PFEXCEPTION_RAISE("DeviceFileAccessError", msg);
  }
  fd_gpo_ = req.fd;
}

void GPIO_HcalHGCROCZCU::setGPO(int ibit, bool toTrue) {
  if (ibit < 0 || ibit >= getGPOcount()) {
    PFEXCEPTION_RAISE("GPIOError", "Requested bit out of range");
  }
  gpio_v2_line_values req;
  memset(&req, 0, sizeof(req));
  req.mask = 1 << ibit;

  if (toTrue)
    req.bits = req.mask;
  else
    req.bits = 0;

  if (ioctl(fd_gpo_, GPIO_V2_LINE_SET_VALUES_IOCTL, &req)) {
    char msg[100];
    snprintf(msg, 100, "Error in writing GPO %d : %d", ibit, errno);
    PFEXCEPTION_RAISE("DeviceFileAccessError", msg);
  }
}

void GPIO_HcalHGCROCZCU::setGPO(const std::vector<bool>& bits) {
  if (int(bits.size()) != getGPOcount()) {
    PFEXCEPTION_RAISE("GPIOError", "Requested bits out of range");
  }

  gpio_v2_line_values req;
  memset(&req, 0, sizeof(req));
  req.mask = 0xFF;
  req.bits = 0;

  for (int i = 0; i < getGPOcount(); i++)
    if (bits[i]) req.bits |= (1 << i);

  if (ioctl(fd_gpo_, GPIO_V2_LINE_SET_VALUES_IOCTL, &req)) {
    char msg[100];
    snprintf(msg, 100, "Error in writing GPOs : %d", errno);
    PFEXCEPTION_RAISE("DeviceFileAccessError", msg);
  }
}

std::vector<bool> GPIO_HcalHGCROCZCU::getGPO() {
  // GPOs are lines 0-7
  std::vector<bool> retval(getGPOcount(), false);
  gpio_v2_line_values req;
  memset(&req, 0, sizeof(req));
  req.mask = 0xFF;
  if (ioctl(fd_gpo_, GPIO_V2_LINE_GET_VALUES_IOCTL, &req)) {
    char msg[100];
    snprintf(msg, 100, "Error in reading GPOs : %d", errno);
    PFEXCEPTION_RAISE("DeviceFileAccessError", msg);
  }

  for (int i = 0; i < getGPOcount(); i++) {
    bool bval = (req.bits & (1 << i)) != 0;
    retval[i] = bval;
  }
  return retval;
}

std::vector<bool> GPIO_HcalHGCROCZCU::getGPI() {
  // GPIs are lines 8
  std::vector<bool> retval;
  gpio_v2_line_values req;
  memset(&req, 0, sizeof(req));
  req.mask = 0x800;
  if (ioctl(gpiodev_, GPIO_V2_LINE_GET_VALUES_IOCTL, &req)) {
    char msg[100];
    snprintf(msg, 100, "Error in reading GPIs : %d", errno);
    PFEXCEPTION_RAISE("DeviceFileAccessError", msg);
  }

  for (int i = 0; i < getGPIcount(); i++)
    retval.push_back((req.bits & (1 << (i + getGPOcount()))) != 0);
  return retval;
}

GPIO* make_GPIO_HcalHGCROCZCU() { return new GPIO_HcalHGCROCZCU(); }
}  // namespace pflib
