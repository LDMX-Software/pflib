#include "pflib/lpgbt/I2C.h"

namespace pflib {
  namespace lpgbt {

    void I2C::set_bus_speed(int speed) {
      ispeed_=speed;
      lpgbt_.setup_i2c(ibus_,speed);
    }

    int I2C::get_bus_speed() { return ispeed_; }
    
    void I2C::write_byte(uint8_t i2c_dev_addr, uint8_t data) {
      lpgbt_.i2c_write(ibus_,i2c_dev_addr,data);
      lpgbt_.i2c_transaction_check(ibus_,true);
    }
    uint8_t I2C::read_byte(uint8_t i2c_dev_addr) {
      lpgbt_.start_i2c_read(ibus_,i2c_dev_addr,1);
      lpgbt_.i2c_transaction_check(ibus_,true);
      std::vector<uint8_t> data=lpgbt_.i2c_read_data(ibus_);
      return data[0];
    }
    std::vector<uint8_t> I2C::general_write_read(uint8_t i2c_dev_addr, const std::vector<uint8_t>& wdata,int nread) {
      if (!wdata.empty()) {
	if (wdata.size()==1) lpgbt_.i2c_write(ibus_,i2c_dev_addr,wdata[0]);
	else lpgbt_.i2c_write(ibus_,i2c_dev_addr,wdata);
	lpgbt_.i2c_transaction_check(ibus_,true);
      }
      if (nread>0) {
	lpgbt_.start_i2c_read(ibus_,i2c_dev_addr,nread);
	lpgbt_.i2c_transaction_check(ibus_,true);
	return lpgbt_.i2c_read_data(ibus_);
      }
      std::vector<uint8_t> rv;
      return rv;
    }


    void I2CwithMux::write_byte(uint8_t i2c_dev_addr, uint8_t data) {
      ::pflib::lpgbt::I2C::write_byte(muxaddr_, wval_);
      ::pflib::lpgbt::I2C::write_byte(i2c_dev_addr, data);
    }
    uint8_t I2CwithMux::read_byte(uint8_t i2c_dev_addr) {
      ::pflib::lpgbt::I2C::write_byte(muxaddr_, wval_);
      return ::pflib::lpgbt::I2C::read_byte(i2c_dev_addr);
    }
    std::vector<uint8_t> I2CwithMux::general_write_read(uint8_t i2c_dev_addr, const std::vector<uint8_t>& wdata,int nread) {
      ::pflib::lpgbt::I2C::write_byte(muxaddr_, wval_);
      return ::pflib::lpgbt::I2C::general_write_read(i2c_dev_addr,wdata,nread);
    }


    
  }
}
