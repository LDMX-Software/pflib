#include "pflib/I2C_Linux.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "pflib/packing/Hex.h"

namespace pflib {

using packing::hex;

I2C_Linux::I2C_Linux(const std::string& device) {
  dev_ = device;
  handle_ = open(device.c_str(), O_RDWR);
  if (handle_ < 0) {
    std::stringstream msg{"Unable to open "};
    msg << device << " with errno " << errno;
    PFEXCEPTION_RAISE("DeviceFileAccessError", msg.str());
  }
  pflib_log(debug) << "opening " << dev_ << " at " << handle_;
}

I2C_Linux::~I2C_Linux() {
  if (handle_ > 0) close(handle_);
  handle_ = 0;
}

void I2C_Linux::set_bus_speed(int speed) {
  // can't actually do this!
}

int I2C_Linux::get_bus_speed() {
  return 100;  // only as set in the firmware...
}

void I2C_Linux::obtain_control(uint8_t i2c_dev_addr) {
  int ret = ioctl(handle_, 0x0703, i2c_dev_addr);
  pflib_log(trace) << "ioctl(" << hex(handle_) << ", 0x0703, "
                   << hex(i2c_dev_addr) << ") -> " << ret;
  if (ret < 0) {
    std::stringstream msg{"Unable to obtain control of "};
    msg << hex(i2c_dev_addr) << " on " << dev_ << " errno " << errno;
    switch (errno) {
      case EBADF:
        msg << " (EBADF) " << hex(handle_) << " is not a valid file descriptor";
        break;
      case EFAULT:
        msg << " (EFAULT) arguments reference inaccessible memory";
        break;
      case EINVAL:
        msg << " (EINVAL) invalid inputs: 0x0703 or " << hex(i2c_dev_addr);
        break;
      case ENOTTY:
        msg << " (ENOTTY) operation does not apply to the type " << dev_
            << " references";
        break;
      default:
        msg << " unknown error number";
        break;
    }
    PFEXCEPTION_RAISE("IOCtrl", msg.str());
  }
}

void I2C_Linux::write_byte(uint8_t i2c_dev_addr, uint8_t data) {
  obtain_control(i2c_dev_addr);

  general_write_read(i2c_dev_addr, {data}, 0);
}

uint8_t I2C_Linux::read_byte(uint8_t i2c_dev_addr) {
  obtain_control(i2c_dev_addr);
  
  std::vector<uint8_t> buffer;
  buffer = general_write_read(i2c_dev_addr, {}, 1);

  return buffer[0];
}

std::vector<uint8_t> I2C_Linux::general_write_read(
    uint8_t i2c_dev_addr, const std::vector<uint8_t>& wdata, int nread) {
  __u8* buf;  //Linux specific type-def that is not necessarily uint8_t

  obtain_control(i2c_dev_addr);
  
  struct i2c_msg msgs[I2C_RDRW_IOCTL_MAX_MSGS];
  for (int i = 0; i < I2C_RDRW_IOCTL_MAX_MSGS; i++) msgs[i].buf = NULL;
  struct i2c_rdwr_ioctl_data rdwr;

  int msg_i = 0;

  if (wdata.size() > 0) {
    msgs[msg_i].addr = i2c_dev_addr;
    msgs[msg_i].flags = 0;
    msgs[msg_i].len = wdata.size();
    buf = (__u8*)malloc(wdata.size());
    if (buf == NULL) {
      PFEXCEPTION_RAISE("I2CError", "Could not malloc buffer");
    }
    memset(buf, 0, wdata.size());
    msgs[msg_i].buf = buf;
    unsigned buf_idx = 0;
    while (buf_idx < wdata.size()) {
      __u8 data = wdata.at(buf_idx);
      msgs[msg_i].buf[buf_idx++] = data;
    }
    msg_i++;
  }

  if (nread > 0) {
    msgs[msg_i].addr = i2c_dev_addr;
    msgs[msg_i].flags = I2C_M_RD;
    msgs[msg_i].len = nread;
    buf = (__u8*)malloc(nread);
    if (buf == NULL) {
      PFEXCEPTION_RAISE("I2CError", "Could not malloc buffer");
    }
    memset(buf, 0, nread);
    msgs[msg_i].buf = buf;
    msg_i++;
  }

  rdwr.msgs = msgs;
  rdwr.nmsgs = msg_i;
  int nmsgs_sent = ioctl(handle_, I2C_RDWR, &rdwr);

  if (nmsgs_sent < 0) {
    PFEXCEPTION_RAISE("I2CError", strerror(errno));
  }

  std::vector<uint8_t> ret;
  for (int i = 0; i < msg_i; i++) {
    int read = !!(msgs[i].flags & I2C_M_RD);
    if (msgs[i].len && read) {
      for (int j = 0; j < msgs[i].len; j++) {
        ret.push_back(msgs[i].buf[j]);
      }
    }
  }

  for (int i = 0; i <= msg_i; i++) free(msgs[i].buf);

  return ret;
}

}  // namespace pflib
