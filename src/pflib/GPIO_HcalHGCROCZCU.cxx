#include <fcntl.h>
#include <linux/gpio.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cerrno>
#include <map>

#include "pflib/Exception.h"
#include "pflib/GPIO.h"
#include "pflib/utility/string_format.h"

namespace pflib {

class GPIO_HcalHGCROCZCU : public GPIO {
 public:
  static constexpr int N_GPO = 8;
  static constexpr int N_GPI = 1;

  GPIO_HcalHGCROCZCU() : GPIO() {
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

  virtual std::vector<std::string> getGPOs() {
    std::vector<std::string> retval;
    for (auto i : gpos_) retval.push_back(i.first);
    return retval;
  }
  virtual std::vector<std::string> getGPIs() {
    std::vector<std::string> retval;
    for (auto i : gpis_) retval.push_back(i.first);
    return retval;
  }

  virtual bool getGPI(const std::string& name);
  virtual bool getGPO(const std::string& name);
  static constexpr const char* gpo_names[] = {
      "HGCROC_RSTB_I2C", "HGCROC_SOFT_RSTB", "HGCROC_HARD_RSTB", "UNUSED1",
      "HGCROC_ADDR0",    "HGCROC_ADDR1",     "SCL_PHASE",        "POWER_OFF"};

  virtual void setGPO(const std::string& name, bool toTrue = true);

 private:
  void setup();
  std::map<std::string, int> gpos_;
  std::map<std::string, int> gpis_;

  int gpiodev_;
  int fd_gpo_;
};

static const char* myname = "PFLIB_GPIO_HCALROCZCU";

void GPIO_HcalHGCROCZCU::setup() {
  gpio_v2_line_request req;

  // clear out the request memory space
  memset(&req, 0, sizeof(req));

  // gpo request
  for (int i = 0; i < N_GPO; i++) {
    req.offsets[i] = i;
    gpos_[gpo_names[i]] = i;
  }
  strncpy(req.consumer, myname, GPIO_MAX_NAME_SIZE);
  req.num_lines = N_GPO;
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

void GPIO_HcalHGCROCZCU::setGPO(const std::string& name, bool toTrue) {
  auto ptr = gpos_.find(name);
  if (ptr == gpos_.end()) {
    PFEXCEPTION_RAISE("GPIOError", pflib::utility::string_format(
                                       "Unknown GPO bit '%s'", name.c_str()));
  }
  int ibit = ptr->second;

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

bool GPIO_HcalHGCROCZCU::getGPO(const std::string& name) {
  auto ptr = gpos_.find(name);
  if (ptr == gpos_.end()) {
    PFEXCEPTION_RAISE("GPIOError", pflib::utility::string_format(
                                       "Unknown GPO bit '%s'", name.c_str()));
  }
  int ibit = ptr->second;

  // GPOs are lines 0-7
  gpio_v2_line_values req;
  memset(&req, 0, sizeof(req));
  req.mask = 0xFF;
  if (ioctl(fd_gpo_, GPIO_V2_LINE_GET_VALUES_IOCTL, &req)) {
    char msg[100];
    snprintf(msg, 100, "Error in reading GPOs : %d", errno);
    PFEXCEPTION_RAISE("DeviceFileAccessError", msg);
  }

  return (req.bits & (1 << ibit)) != 0;
}

bool GPIO_HcalHGCROCZCU::getGPI(const std::string& name) {
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

  return (req.bits & 0x800) != 0;
}

GPIO* make_GPIO_HcalHGCROCZCU() { return new GPIO_HcalHGCROCZCU(); }
}  // namespace pflib
