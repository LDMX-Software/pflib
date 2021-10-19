#ifndef PFLIB_WISHBONETARGET_H_
#define PFLIB_WISHBONETARGET_H_

#include "pflib/WishboneInterface.h"

namespace pflib {

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
 *@class WishboneTarget
 *@brief Parent class for standard wishbone targets providing some utilities
 */
class WishboneTarget {
 public:
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
  int firmware_verion();

 protected:

  void wb_write(uint32_t addr, uint32_t data) { wb_->wb_write(target_,addr,data); }
  uint32_t wb_read(uint32_t addr) { return wb_->wb_read(target_,addr); }

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
  
