#ifndef LPGBT_OPTOLINK_INCLUDED
#define LPGBT_OPTOLINK_INCLUDED 1

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "pflib/lpGBT.h"

namespace pflib {

/** Represents an interface to the optical links (GTX, GTH, GTY)
    and the upper levels of the encoder/decoder blocks
*/
class OptoLink {
 public:
  /// which optical link does this represent?
  virtual int ilink() = 0;
  /// is this link bidirectional (e.g. a DAQ lpGBT)
  virtual bool is_bidirectional() { return true; }
  /// reset a given optical link, may have side-effects
  virtual void reset_link() = 0;

  virtual void run_linktrick() {}
  virtual bool get_rx_polarity() = 0;
  virtual bool get_tx_polarity() = 0;
  virtual void set_rx_polarity(bool polarity) = 0;
  virtual void set_tx_polarity(bool polarity) = 0;

  virtual std::map<std::string, uint32_t> opto_status() = 0;
  virtual std::map<std::string, uint32_t> opto_rates() = 0;

  // get the necessary transport for creating an lpGBT object for the given link
  virtual lpGBT_ConfigTransport& lpgbt_transport() = 0;

  // get an lpGBT object using the transport
  lpGBT& lpgbt() {
    if (!lpgbt_) lpgbt_ = std::make_unique<lpGBT>(lpgbt_transport());
    return *lpgbt_;
  }

  // setup various aspects
  /// there are four TX elinks configured in the coder block, this is needed for
  /// testing purposes only
  virtual int get_elink_tx_mode(int elink) = 0;
  virtual void set_elink_tx_mode(int elink, int mode) = 0;

  virtual void capture_ec(int mode, std::vector<uint8_t>& tx,
                          std::vector<uint8_t>& rx) = 0;
  virtual void capture_ic(int mode, std::vector<uint8_t>& tx,
                          std::vector<uint8_t>& rx) = 0;

 private:
  std::unique_ptr<lpGBT> lpgbt_;
};

}  // namespace pflib

#endif  // LPGBT_OPTOLINKS_INCLUDED
