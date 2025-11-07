#ifndef ZCU_OPTOLINK_INCLUDED
#define ZCU_OPTOLINK_INCLUDED 1

#include "pflib/OptoLink.h"
#include "pflib/zcu/UIO.h"
#include "pflib/zcu/lpGBT_ICEC_ZCU_Simple.h"
#include <memory>

namespace pflib {
namespace zcu {

  class ZCUOptoLink : public pflib::OptoLink {
  public:
    ZCUOptoLink(const std::string& name = "singleLPGBT", int ilink=0, bool isdaq=true);

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

    ::pflib::UIO& coder() { return coder_; }
    
    // setup various aspects
    /// there are four TX elinks configured in the coder block
    virtual int get_elink_tx_mode(int elink);
    virtual void set_elink_tx_mode(int elink, int mode);

    virtual void capture_ec(int mode, std::vector<uint8_t>& tx, std::vector<uint8_t>& rx);
    virtual void capture_ic(int mode, std::vector<uint8_t>& tx, std::vector<uint8_t>& rx);

  private:
    std::unique_ptr<::pflib::zcu::lpGBT_ICEC_Simple> transport_;
    ::pflib::UIO transright_;
    ::pflib::UIO coder_;
    std::string coder_name_;
    int ilink_;
    bool isdaq_;
  };

}  // namespace zcu
}  // namespace pflib

#endif  // ZCU_OPTOLINK_INCLUDED
