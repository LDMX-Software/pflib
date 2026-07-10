#ifndef PFLIB_ZCU_TRIG_H_INCLUDED
#define PFLIB_ZCU_TRIG_H_INCLUDED

#include "pflib/TRIG.h"
#include "pflib/zcu/UIO.h"

namespace pflib {
namespace zcu {

class ZCUtrig : public TRIG {
 public:
  ZCUtrig();
  void reset();
  int n_elinks() const override { return nelinks_; }

  void setup_alignment_capture(int delay) override;
  int get_alignment_capture() override;

  std::vector<uint32_t> read_capture_buffer(int ilink) override;

  void set_bx_delay(int ilink, int delay) override;

  int get_bx_delay(int ilink) override;

  void setup_daq(int pipeline, int econ_id, int samples_per_l1a,
                         int presamples) override;

  void get_daq_setup(int& pipeline, int& econ_id, int& samples_per_l1a,
                             int& presamples) override;

  bool is_event_available() override;

  std::vector<uint32_t> read_event() override;

 private:
  UIO uio_;
  int nelinks_;
};

}  // namespace zcu
}  // namespace pflib

#endif  // PFLIB_ZCU_TRIG_H_INCLUDED
