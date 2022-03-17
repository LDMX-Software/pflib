#ifndef PFLIB_BACKEND_H_
#define PFLIB_BACKEND_H_

#include <pflib/Exception.h>
#include <stdint.h>
#include <vector>

namespace pflib {

/**
 * Abstract interface for various backend manipulations
 *
 * All backend communication methods need to implement these
 * functions.
 */
class Backend {
public:
  /** send a single L1A */
  virtual void fc_sendL1A() = 0;

  /** send a link reset */
  virtual void fc_linkreset() = 0;
  
  /** send a single L1A */
  virtual void fc_bufferclear() = 0;

  /** send a single calib pulse */
  virtual void fc_calibpulse() = 0;

  /** calib pulse setup */
  virtual void fc_setup_calib(int pulse_len, int l1a_offset) = 0;

  /** calib pulse setup */
  virtual void fc_get_setup_calib(int& pulse_len, int& l1a_offset) = 0;

  /** reset the daq buffers */
  virtual void daq_reset() = 0;

  /** advance the daq pointer along buffer */
  virtual void daq_advance_ptr() = 0;
  
  /** readout the daq status into the passed variables */
  virtual void daq_status(bool& full, bool& empty, int& nevents, int& next_event_size) = 0;
  
  /** read the aquired event and return it */
  virtual std::vector<uint32_t> daq_read_event() = 0;  

  /** Set the event tagging information */
  virtual void daq_setup_event_tag(int run, int day, int month, int hour, int min);
};

}

#endif
