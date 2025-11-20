#ifndef BITTWARE_OPTOLINK_INCLUDED
#define BITTWARE_OPTOLINK_INCLUDED 1

#include <memory>

#include "pflib/OptoLink.h"
#include "pflib/bittware/bittware_axilite.h"
#include "pflib/lpGBT.h"

namespace pflib {
namespace bittware {

class BWlpGBT_Transport : public pflib::lpGBT_ConfigTransport {
 public:
  BWlpGBT_Transport(AxiLite& coder, int ilink, int chipaddr, bool isic = true);
  virtual uint8_t read_reg(uint16_t reg);
  virtual std::vector<uint8_t> read_regs(uint16_t reg, int n);
  virtual void write_reg(uint16_t reg, uint8_t value);
  virtual void write_regs(uint16_t reg, const std::vector<uint8_t>& value);

 private:
  AxiLite& transport_;
  int ilink_;
  int chipaddr_;
  bool isic_;
  int ctloffset_, stsreg_;
  uint32_t stsmask_;
  int pulsereg_, pulseshift_;
};

class BWOptoLink : public pflib::OptoLink {
 public:
  /// for a daq link
  BWOptoLink(int ilink, const char* dev);
  /// for a trigger link
  BWOptoLink(int ilink, BWOptoLink& daqlink);

  const char* dev() const { return gtys_.dev(); }
  virtual int ilink() { return ilink_; }
  virtual bool is_bidirectional() { return isdaq_; }
  virtual void reset_link();
  virtual void run_linktrick();

  virtual bool get_rx_polarity();
  virtual bool get_tx_polarity();
  virtual void set_rx_polarity(bool polarity);
  virtual void set_tx_polarity(bool polarity);

  virtual std::map<std::string, uint32_t> opto_status();
  virtual std::map<std::string, uint32_t> opto_rates();

  virtual lpGBT_ConfigTransport& lpgbt_transport() { return *transport_; }

  // setup various aspects
  /// there are four TX elinks configured in the coder block
  virtual int get_elink_tx_mode(int elink);
  virtual void set_elink_tx_mode(int elink, int mode);

  virtual void capture_ec(int mode, std::vector<uint8_t>& tx,
                          std::vector<uint8_t>& rx);
  virtual void capture_ic(int mode, std::vector<uint8_t>& tx,
                          std::vector<uint8_t>& rx);

 private:
  AxiLite gtys_;
  std::shared_ptr<AxiLite> coder_;
  std::shared_ptr<AxiLite> iceccoder_;
  int ilink_;
  bool isdaq_;
  std::unique_ptr<BWlpGBT_Transport> transport_;
};

}  // namespace bittware
}  // namespace pflib

#endif  // BITTWARE_OPTOLINK_INCLUDED
