#ifndef PFLIB_FastControl_H_
#define PFLIB_FastControl_H_

#include "pflib/WishboneTarget.h"
#include <vector>

namespace pflib {

/**
 * Representation of FastControl controller in the Polarfire
 */
class FastControl : public WishboneTarget {
 public:
  FastControl(WishboneInterface* wb, int target = tgt_FastControl) 
    : WishboneTarget(wb,target) {}

  /** 
   * Get the counters for all the different fast control commands
   */
  std::vector<uint32_t> getCmdCounters();

  /** 
   * Get the error counters 
   */
  void getErrorCounters(uint32_t& single_bit_errors, uint32_t& double_bit_errors);

  /**
   * Reset the command and error counters
   */
  void resetCounters();

  /**
   * Reset the transmitter PLL
   */
  void resetTransmitter();

  /** 
   * Enable/disable multisample readout
   */
  void setupMultisample(bool enable, int extra_samples);

  /** 
   * Get the current multisample readout status
   */
  void getMultisampleSetup(bool& enable, int& extra_samples);

};

}

#endif // PFLIB_FastControl_H_
