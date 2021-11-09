#ifndef PFLIB_WISHBONEINTERFACE_H_
#define PFLIB_WISHBONEINTERFACE_H_

#include <pflib/Exception.h>
#include <stdint.h>

namespace pflib {

/**
 * @class WishboneInterface
 * @brief Abstract interface for wishbone transactions, used by ~all classes
 * in pflib.
 *
 * Perfectly C++ abstract base classes are very difficult to bind
 * to python, so we simply define the base class to have nothing
 * happen in all of these functions.
 */
class WishboneInterface {
 public:
  /**
   * write a 32-bit word to the given target and address
   * @throws pflib::Exception in the case of a timeout or wishbone error
   */
  virtual void wb_write(int target, uint32_t addr, uint32_t data) {}
  /**
   * read a 32-bit word from the given target and address
   * @throws pflib::Exception in the case of a timeout or wishbone error
   */
  virtual uint32_t wb_read(int target, uint32_t addr) { return uint32_t(); }
  /**
   * reset the wishbone bus (on/off cycle)
   */
  virtual void wb_reset() {}

  /** 
   * Read the monitoring counters
   *  crcup_errors -- CRC errors observed on the uplink (wraps)
   *  crcdn_errors -- CRC errors from the downlink reported on the uplink (wraps)
   *  wb_errors -- Wishbone errors indicated by the Polarfire
   */
  virtual void wb_errors(uint32_t& crcup_errors, uint32_t& crcdn_errors, uint32_t& wb_errors) {}

  /**
   * Clear the monitoring counters
   */
  virtual void wb_clear_counters() {}
  
};

}

#endif // PFLIB_WISHBONEINTERFACE_H_
