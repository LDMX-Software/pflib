#ifndef PFLIB_WISHBONETARGET_H_
#define PFLIB_WISHBONETARGET_H_

#include "pflib/WishboneInterface.h"

namespace pflib {

/**
 * common targets for the wishbone connection
 */
enum StandardWishboneTargets {
  tgt_COMMON      = 0,
  tgt_FastControl = 1,
  tgt_GPIO        = 2,
  tgt_I2C         = 3,
  tgt_Elinks      = 4,
  tgt_DAQ_Control = 8,
  tgt_DAQ_Inbuffer = 9,
  tgt_DAQ_LinkFmt  = 10,
  tgt_DAQ_Outbuffer= 11
};

/**
 * Parent class for standard wishbone targets providing some utilities
 *
 * Simply wrap the wishbone target integer so that we can call addr/data
 * read/writes without having to always provide the target.
 */
class WishboneTarget {
 public:
  /**
   * simply store pointer to interface and target we will interact with
   */
  WishboneTarget(WishboneInterface* wb, int target) : wb_{wb}, target_{target} { }

  /** 
   * Get the Wishbone target type/flavor by number
   */
  int target_type();

  /**
   * Get the Wishbone target type/flavor by name if known
   */
  std::string target_type_str();

  /**
   * Get the subblock firmware version
   */
  int firmware_version();

 protected:

  /**
   * Write data to an address on the target stored within us
   *
   * @see WishboneInterface::wb_write for writing data via interface
   *
   * @param[in] addr address of data to write
   * @param[in] data data to write
   */
  void wb_write(uint32_t addr, uint32_t data) { wb_->wb_write(target_,addr,data); }

  /**
   * Read data from address on the target stored within us
   *
   * @see WishboneInterface::wb_read for reading via interface
   * 
   * @param[in] addr address of data to read
   * @return data word at that address
   */
  uint32_t wb_read(uint32_t addr) { return wb_->wb_read(target_,addr); }

  /**
   * Read the word at the address and then put our data bits into
   * that address using the provided mask.
   *
   * @param[in] addr address to insert data into
   * @param[in] data data we want to put
   * @param[in] mask mask for data
   */
  void wb_rmw(uint32_t addr, uint32_t data, uint32_t mask);
    
  /**
   * Handle to the interface
   */
  WishboneInterface* wb_;

  /** 
   * Target id
   */
  int target_;
};
}

#endif // PFLIB_WISHBONE_TARGET_H_
  
