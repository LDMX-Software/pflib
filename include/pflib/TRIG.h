#ifndef PFLIB_TRIG_H_INCLUDED
#define PFLIB_TRIG_H_INCLUDED 1

#include <stdint.h>
#include <vector>

namespace pflib {

/** Trigger path management firmware
    
    In full-detector configuration, the trigger path is handled
    mostly-independently from PFLIB.  However, for teststand
    and debugging, the trigger path may be integrated into the same
    FPGA as the DAQ, motivating this software block.
*/
class TRIG {
 public:
  /** Reset the internals */
  virtual void reset() { }
  
  /** How many elinks are there? */
  virtual int n_elinks() const = 0;
  
  /** Set up the alignment capture function */
  virtual void setup_alignment_capture(int delay) = 0;

  /** Read the capture block for the given elink */
  virtual std::vector<uint32_t> read_capture_buffer(int ilink) = 0;

  /** Set the BX delay for the given elink */
  virtual void set_bx_delay(int ilink, int delay) = 0;

  /** Get the BX delay for the given elink */
  virtual int get_bx_delay(int ilink) = 0;

  /** Setup the DAQ path */
  virtual void setup_daq(int pipeline, int econ_id, int samples_per_l1a=1, int presamples=0) = 0;

  /** Setup the DAQ path */
  virtual void get_daq_setup(int& pipeline, int& econ_id, int& samples_per_l1a, int& presamples) = 0;
 
  /** Is there an event available? */
  virtual bool is_event_available() = 0;

  /** Read the next event */
  virtual std::vector<uint32_t> read_event() = 0;
      
};

}

#endif // PFLIB_TRIG_H_INCLUDED
