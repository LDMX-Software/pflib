#ifndef PFLIB_FastControl_H_
#define PFLIB_FastControl_H_

#include <vector>
#include <cstdint>

namespace pflib {

/**
 * Representation of FastControl controller
 */
class FastControl {
 public:
  /**
   * Get the counters for all the different fast control commands
   */
  virtual std::vector<uint32_t> getCmdCounters() = 0;

  /**
   * clear the counters
   */
  virtual void resetCounters() { }
  
  /** send a single L1A */
  virtual void sendL1A() = 0;

  /** send a link reset */
  virtual void linkreset_rocs() = 0;

  /** send a buffer clear */
  virtual void bufferclear() = 0;

  /** send a single calib pulse */
  virtual void chargepulse() = 0;

  /** send a single calib pulse */
  virtual void ledpulse() = 0;

  /** reset counters for a new run */
  virtual void clear_run() {}

  /** calib pulse setup */
  virtual void fc_setup_calib(int charge_to_l1a) {}

  /** calib pulse setup (charge to l1a time) */
  virtual int fc_get_setup_calib() { return -1; }

  /** read counters from the FC side */
  virtual void read_counters(int& spill_count, int& header_occ,
                             int& event_count, int& vetoed_counter) {}

  /** check the enables for various trigger/spill sources */
  virtual void fc_enables_read(bool& ext_l1a, bool& ext_spill,
                               bool& timer_l1a) {}

  /** set the enables for various trigger/spill sources */
  virtual void fc_enables(bool ext_l1a, bool ext_spill, bool timer_l1a) {}

  /** get the period in us for the timer trigger */
  virtual int fc_timer_setup_read() { return -1; }

  /** set the period in us for the timer trigger */
  virtual void fc_timer_setup(int usdelay) {}
};

// factories
FastControl* make_FastControlCMS_MMap();

}  // namespace pflib

#endif  // PFLIB_FastControl_H_
