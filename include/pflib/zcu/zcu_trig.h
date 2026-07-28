#ifndef PFLIB_ZCU_TRIG_H_INCLUDED
#define PFLIB_ZCU_TRIG_H_INCLUDED

#include "pflib/TRIG.h"
#include "pflib/logging/Logging.h"
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

  bool is_sample_available() override;
  std::vector<uint32_t> read_sample() override;

  void setup_algo(const std::vector<uint32_t>& parameters) override;
  std::vector<uint32_t> get_algo_setup() override;
  bool is_algo_output_available() override;
  std::vector<uint32_t> read_algo_output_sample() override;

  bool get_enable_single_shot() override;
  int get_self_trigger_count() override;
  void enable_single_shot(bool enable = true) override;
  bool single_shot_fired() override;
  void reset_single_shot() override;

  void clear_histograms();
  void debug_histogram(int i);
  std::vector<uint32_t> read_histogram(int ihist);

 private:
  UIO uio_;
  int nelinks_;
  mutable logging::logger the_log_;
};

}  // namespace zcu
}  // namespace pflib

#endif  // PFLIB_ZCU_TRIG_H_INCLUDED
