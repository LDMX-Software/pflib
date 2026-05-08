#ifndef PFLIB_ZCU_TRIG_H_INCLUDED
#define PFLIB_ZCU_TRIG_H_INCLUDED

#include "pflib/TRIG.h"

#include "pflib/zcu/UIO.h"

namespace pflib {
namespace zcu {

class ZCUtrig : public TRIG {
 public:
  ZCUtrig();
  virtual void reset();
  virtual int n_elinks() const { return nelinks_; }
  
  virtual void setup_alignment_capture(int delay);

  virtual std::vector<uint32_t> read_capture_buffer(int ilink);

  virtual void set_bx_delay(int ilink, int delay);

  virtual int get_bx_delay(int ilink);

  virtual void setup_daq(int pipeline, int econ_id, int samples_per_l1a, int presamples);

  virtual void get_daq_setup(int& pipeline, int& econ_id, int& samples_per_l1a, int& presamples);
 
  virtual bool is_event_available();

  virtual std::vector<uint32_t> read_event();

 private:
  UIO uio_;
  int nelinks_;
  
  
};
  
}
}

#endif // PFLIB_ZCU_TRIG_H_INCLUDED
