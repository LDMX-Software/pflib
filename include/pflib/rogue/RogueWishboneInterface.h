#ifndef ROGUE_WISHBONE_INTERFACE_H_
#define ROGUE_WISHBONE_INTERFACE_H_ 1

#include "pflib/WishboneInterface.h"
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

class RogueWishboneInterface : public WishboneInterface {
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

 private:
  std::shared_ptr<::rogue::interfaces::memory::TcpClient> client_;
  std::shared_ptr<::rogue::interfaces::memory::Master> intf_;  
};

}
}


#endif // ROGUE_WISHBONE_INTERFACE_H_
