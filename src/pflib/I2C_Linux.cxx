#include "pflib/I2C_Linux.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <errno.h>

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

static int n_tries = 4;

void I2C_Linux::write_byte(uint8_t i2c_dev_addr, uint8_t data) {
  obtain_control(i2c_dev_addr);

  int rv{-1};
  for (std::size_t i_try{0}; i_try < n_tries; i_try++) {
    rv = write(handle_, &data, 1);
    if (rv >= 0) {
      pflib_log(trace) << "wrote " << hex(data) << " to " << hex(i2c_dev_addr);
      // success! let's leave
      return;
    }
  }
  // still erroring out (rv < 0) and we ran out of tries
  char message[120];
  snprintf(message, 120, "Error %d writing 0x%02x to i2c target 0x%02x", errno,
           data, i2c_dev_addr);
  PFEXCEPTION_RAISE("I2CError", message);
}

uint8_t I2C_Linux::read_byte(uint8_t i2c_dev_addr) {
  obtain_control(i2c_dev_addr);
  uint8_t buffer[8];
  int rv{-1};
  for (std::size_t i_try{0}; i_try < n_tries; i_try++) {
    rv = read(handle_, buffer, 1);
    pflib_log(trace) << "read(" << hex(handle_) << ", " << hex(buffer)
                     << ", 1) -> " << rv << " errno " << errno;
    if (rv >= 0) {
      pflib_log(trace) << "read " << hex(buffer[0]) << " from "
                       << hex(i2c_dev_addr);
      // success! let's leave
      return buffer[0];
    }
  }
  std::stringstream msg;
  msg << errno;
  switch (errno) {
    case EAGAIN:
      msg << " (EAGAIN) " << dev_ << "(" << hex(handle_)
          << ") is marked as non blocking";
      break;
    case EBADF:
      msg << " (EBADF) " << dev_ << "(" << hex(handle_)
          << ") is not a valid descriptor or not open for reading";
      break;
    case EFAULT:
      msg << " (EFAULT) buffer is outside accessible address space";
      break;
    case EINTR:
      msg << " (EINTR) signal interrupted call";
      break;
    case EINVAL:
      msg << " (EINVAL) buffer address or file offset is not aligned";
      break;
    case EIO:
      msg << " (EIO) generic I/O error";
      break;
    case EISDIR:
      msg << " (EISDIR) " << dev_ << "(" << hex(handle_)
          << ") refers to a directory";
      break;
  }
  PFEXCEPTION_RAISE("I2CError", msg.str());
  return buffer[0];
}

std::vector<uint8_t> I2C_Linux::general_write_read(
    uint8_t i2c_dev_addr, const std::vector<uint8_t>& wdata, int nread) {
  //ioctl(handle_, 0x0706, i2c_dev_addr);
  obtain_control(i2c_dev_addr);

  if (wdata.size() > 0) {
    int ret = write(handle_, &(wdata[0]), wdata.size());

    if (ret < 0) {
      char message[120];
      snprintf(message, 120, "Error %d writing to i2c target 0x%02x", errno,
               i2c_dev_addr);
      PFEXCEPTION_RAISE("I2CError", message);
    }
  }

  std::vector<uint8_t> rv(nread, 0);

  if (nread > 0) {
    int ret = read(handle_, &rv[0], nread);

    if (ret < 0) {
      char message[120];
      snprintf(message, 120, "Error %d reading from i2c target 0x%02x", errno,
               i2c_dev_addr);
      PFEXCEPTION_RAISE("I2CError", message);
    }
  }

  return rv;
}

std::vector<uint8_t> I2C_Linux::general_write_read_ioctl(int i2c_dev_addr, const std::vector<uint8_t>& wdata, int nread){
  
  __u8* buf; //Linux specific type-def that is not necessarily uint8_t
  
  struct i2c_msg msgs[I2C_RDRW_IOCTL_MAX_MSGS];
  for (int i = 0; i < I2C_RDRW_IOCTL_MAX_MSGS; i++)
    msgs[i].buf = NULL;
  struct i2c_rdwr_ioctl_data rdwr;

  int msg_i = 0;

  if(wdata.size() > 0){
    msgs[msg_i].addr = i2c_dev_addr;
    msgs[msg_i].flags = 0;
    msgs[msg_i].len = wdata.size();
    buf = (__u8*)malloc(wdata.size());
    if(buf == NULL){
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

  if(nread > 0){
    msgs[msg_i].addr = i2c_dev_addr;
    msgs[msg_i].flags = I2C_M_RD;
    msgs[msg_i].len = nread;
    buf = (__u8*)malloc(nread);
    if(buf == NULL){
      PFEXCEPTION_RAISE("I2CError", "Could not malloc buffer");
    }
    memset(buf, 0, nread);
    msgs[msg_i].buf = buf;
    msg_i++;
  }

  rdwr.msgs = msgs;
  rdwr.nmsgs = msg_i;
  int nmsgs_sent = ioctl(handle_, I2C_RDWR, &rdwr);

  if(nmsgs_sent < 0){
    PFEXCEPTION_RAISE("I2CError", strerror(errno));
  }

  std::vector<uint8_t> ret;
  int read = !!(msgs[1].flags & I2C_M_RD);
  if (msgs[1].len && read) {
    for (int j = 0; j < msgs[1].len; j++){
      ret.push_back(msgs[1].buf[j]);
    }
  }

  for (int i = 0; i <= msg_i; i++)
    free(msgs[i].buf);
 
  return ret; 
}

}  // namespace pflib
