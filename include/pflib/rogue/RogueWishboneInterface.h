#ifndef ROGUE_WISHBONE_INTERFACE_H_
#define ROGUE_WISHBONE_INTERFACE_H_ 1

#include "pflib/WishboneInterface.h"
#include "pflib/Backend.h"
#include <memory>

// Forward declaration of the TCP client
namespace rogue {
namespace interfaces {
namespace memory {
class TcpClient;
class Master;
}
}
}
      
namespace pflib {
namespace rogue {

class RogueWishboneInterface : public WishboneInterface, public Backend {
 public:
  /** Construct a TCP bridge*/
  RogueWishboneInterface(const std::string& host, int port);

  virtual ~RogueWishboneInterface();
  
  /**
   * write a 32-bit word to the given target and address
   * @throws pflib::Exception in the case of a timeout or wishbone error
   */
  virtual void wb_write(int target, uint32_t addr, uint32_t data);
  
  /**
   * read a 32-bit word from the given target and address
   * @throws pflib::Exception in the case of a timeout or wishbone error
   */
  virtual uint32_t wb_read(int target, uint32_t addr);
  
  /**
   * reset the wishbone bus (on/off cycle)
   */
  virtual void wb_reset();

  /** 
   * Read the monitoring counters
   *  crcup_errors -- CRC errors observed on the uplink (wraps)
   *  crcdn_errors -- CRC errors from the downlink reported on the uplink (wraps)
   *  wb_errors -- Wishbone errors indicated by the Polarfire
   */
  virtual void wb_errors(uint32_t& crcup_errors, uint32_t& crcdn_errors, uint32_t& wb_errors);

  /**
   * Clear the monitoring counters
   */
  virtual void wb_clear_counters();


  /// Backend implementation
  virtual void fc_sendL1A();
  virtual void fc_linkreset();
  virtual void fc_bufferclear();
  virtual void fc_calibpulse();
  virtual void fc_setup_calib(int pulse_len, int l1a_offset);
  virtual void fc_get_setup_calib(int& pulse_len, int& l1a_offset);
  
  virtual void daq_reset();
  virtual void daq_advance_ptr();
  virtual void daq_status(bool& full, bool& empty, int& nevents, int& next_event_size);
  virtual std::vector<uint32_t> daq_read_event();  
 private:
  std::shared_ptr<::rogue::interfaces::memory::TcpClient> client_;
  std::shared_ptr<::rogue::interfaces::memory::Master> intf_;  
};

}
}


#endif // ROGUE_WISHBONE_INTERFACE_H_
