#ifndef PFLIB_FastControl_H_
#define PFLIB_FastControl_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace pflib {

/**
 * Representation of FastControl controller
 */
class FastControl {
 public:
  FastControl() : l1a_per_ror_{1} {}

  /**
   * Get the counters for all the different fast control commands
   */
  virtual std::map<std::string, uint32_t> getCmdCounters() = 0;

  /**
   * clear the counters
   */
  virtual void resetCounters() {}

  /**
   * Do standard setup for FastControl interface
   * e.g. constructing fast control commands for requesting later
   */
  virtual void standard_setup() {}

  /** send a single L1A */
  virtual void sendL1A() = 0;

  /** send a single ROR */
  virtual void sendROR() {
    for (int i = 0; i < l1a_per_ror_; i++) sendL1A();
  }

  /** set the number of L1A per ROR */
  virtual void setL1AperROR(int n) { l1a_per_ror_ = n; }

  /** get the number of L1A per ROR */
  virtual int getL1AperROR() { return l1a_per_ror_; }

  /** send a link reset */
  virtual void linkreset_rocs() = 0;

  /** set custom bunch crossing ???? for what??? */
  virtual void bx_custom(int bx_addr, int bx_mask, int bx_new) = 0;

  /** send a link reset to the ECONs*/
  virtual void linkreset_econs() {};

  /** send a buffer clear */
  virtual void bufferclear() = 0;

  /** send a orbit count reset */  // Josh
  virtual void orbit_count_reset() = 0;

  /** send a single calib pulse */
  virtual void chargepulse() = 0;

  /** send a single calib pulse */
  virtual void ledpulse() = 0;

  /** reset counters for a new run */
  virtual void clear_run() {}

  /** setup the link reset timing */
  virtual void fc_setup_link_reset(int bx) {}

  /** setup the link reset timing */
  virtual void fc_get_setup_link_reset(int& bx) {}

  /** calib pulse setup */
  virtual void fc_setup_calib(int charge_to_l1a) {}

  /** calib pulse setup (charge to l1a time) */
  virtual int fc_get_setup_calib() { return -1; }

  /** led pulse setup */
  virtual void fc_setup_led(int charge_to_l1a) {}

  /** led pulse setup (charge to l1a time) */
  virtual int fc_get_setup_led() { return -1; }

  /** read counters from the FC side */
  virtual void read_counters(int& spill_count, int& header_occ,
                             int& event_count, int& vetoed_counter) {}

  /** check the enables for various trigger/spill sources */
  virtual void fc_enables_read(bool& l1a_overall, bool& ext_l1a) {}

  /** set the enables for various trigger/spill sources */
  virtual void fc_enables(bool l1a_overall, bool ext_l1a) {}

  /** get the period in us for the timer trigger */
  virtual int fc_timer_setup_read() { return -1; }

  /** set the period in us for the timer trigger */
  virtual void fc_timer_setup(int usdelay) {}

 protected:
  int l1a_per_ror_;
};

// factories
FastControl* make_FastControlCMS_MMap();

}  // namespace pflib

#endif  // PFLIB_FastControl_H_
