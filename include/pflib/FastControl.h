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
  virtual ~FastControl() = default;

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
  virtual void sendROR() = 0;

  /** set the number of L1A per ROR */
  virtual void setL1AperROR(int n) = 0;

  /** get the number of L1A per ROR */
  virtual int getL1AperROR() = 0;

  /** send a link reset */
  virtual void linkreset_rocs() = 0;

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

  /**
   * setup the orbit blinker L1A
   *
   * @param[in] enable turn on/off orbit blinker
   * @param[in] bx which BX the L1A is sent on
   */
  virtual void fc_setup_orbit_blinker(bool enable, int bx) {}

  /**
   * get the orbit blinker's settings
   *
   * @param[out] enable whether the blinker is on/off
   * @param[out] bx which BX the L1A is sent on
   */
  virtual void fc_get_orbit_blinker(bool& enable, int& bx) {}

  /** setup the link reset timing */
  virtual void fc_setup_link_reset(int bx) {}

  /** setup the link reset timing */
  virtual void fc_get_setup_link_reset(int& bx) {}

  /**
   * calib pulse setup
   *
   * @param[in] charge_to_l1a number of bx separating the charge
   * command from the following L1A command
   * @param[in] enable_follow_l1a whether to enable a following
   * L1A command (true) or not (false)
   *
   * You probably want to enable the following L1A command if
   * you are doing chip-tuning. Disabling it is only helpful
   * if you are testing the ability for some other infrastructure
   * to make the trigger descision.
   */
  virtual void fc_setup_calib(int charge_to_l1a, bool enable_follow_l1a) {}

  /** calib pulse setup (charge to l1a time) */
  virtual void fc_get_setup_calib(int& charge_to_l1a, bool& enable_follow_l1a) {
  }

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
};

// factories
FastControl* make_FastControlCMS_MMap();

}  // namespace pflib

#endif  // PFLIB_FastControl_H_
