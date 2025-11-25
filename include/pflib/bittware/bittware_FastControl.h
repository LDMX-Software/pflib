#ifndef PFLIB_bittware_FastControl_H_
#define PFLIB_bittware_FastControl_H_

#include "pflib/FastControl.h"
#include "pflib/bittware/bittware_axilite.h"

namespace pflib {
namespace bittware {

/**
 * Representation of FastControl controller
 */
class BWFastControl : public FastControl {
 public:
  BWFastControl(const char* dev);
  virtual std::map<std::string, uint32_t> getCmdCounters();
  virtual void resetCounters();
  virtual void sendL1A();
  virtual void sendROR();
  virtual void setL1AperROR(int n);
  virtual int getL1AperROR();
  virtual void linkreset_rocs();

  virtual void bx_custom(int bx_addr, int bx_mask, int bx_new) {}

  virtual void linkreset_econs();
  virtual void bufferclear();
  virtual void orbit_count_reset();
  virtual void chargepulse();
  virtual void ledpulse();
  virtual void clear_run();
  virtual void fc_setup_calib(int charge_to_l1a);
  virtual int fc_get_setup_calib();
  virtual void fc_setup_led(int charge_to_l1a);
  virtual int fc_get_setup_led();
  virtual void fc_setup_link_reset(int bx);
  virtual void fc_get_setup_link_reset(int& bx);

  /** check the enables for various trigger sources */
  virtual void fc_enables_read(bool& all_l1a, bool& ext_l1a);

  /** set the enables for various trigger sources */
  virtual void fc_enables(bool all_l1a, bool ext_l1a);

 private:
  AxiLite axi_;
};

}  // namespace bittware
}  // namespace pflib

#endif  // PFLIB_bittware_FastControl_H_
