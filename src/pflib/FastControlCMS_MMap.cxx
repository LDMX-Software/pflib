/**
   This version of the fast control code interfaces with the CMS
   Fast control library which can be controlled over MMap/UIO
*/
#include <stdint.h>
#include <stdio.h>

#include <ostream>

#include "pflib/Exception.h"
#include "pflib/FastControl.h"
#include "pflib/Logging.h"
#include "pflib/zcu/UIO.h"

namespace pflib {

static const size_t ADDR_CTL_REG = 0;
static const size_t ADDR_REQUEST = 1;

static const uint32_t REQ_CLEAR_COUNTERS = 0x2;
static const uint32_t CTL_ENABLE_ORBITSYNC = 0x0004;
static const uint32_t CTL_ENABLE_L1AS = 0x0008;

/*
struct FastControlAXI {
uint32_t ctl_reg;
uint32_t request;
uint32_t bx_chipsync_orbitsync;
uint32_t bx_linkreset_rocd_roct;
uint32_t bx_linkreset_econd_econt;
uint32_t bx_ecr_ebr;
uint32_t bx_spares[4];
uint32_t trigger_filters;
uint32_t external_trigger_delay; // 11
uint32_t random_trigger_log2;
uint32_t zs_per_nzs;
uint32_t cmdseq_bx_len;
uint32_t cmdseq_prescale;
uint32_t command_delay; // don't use me!
};
*/

class Periodic {
 public:
  bool enable;
  int flavor;
  bool enable_follow;
  int follow_which;
  int bx;
  int orbit_prescale;
  int burst_length;

  Periodic(UIO& uio, int offset) : uio_{uio}, offset_(offset) { reload(); }
  void reload() {
    uint32_t a = uio_.read(offset_);
    enable = a & 0x1;
    enable_follow = a & 0x4;
    flavor = (a & 0x38) >> 3;
    follow_which = (a & 0xF00000) >> 20;
    bx = (a & 0xFFF00) >> 8;
    orbit_prescale = (uio_.read(offset_ + 1) & 0xFFFFF);
    burst_length = (uio_.read(offset_ + 1) >> 20) & 0x3FF;
  }
  void request() { uio_.rmw(offset_ + 0, 0x2, 0x2); }
  friend std::ostream& operator<<(std::ostream& o, const Periodic& p) {
    o << "{ " << " enable: " << p.enable << ','
      << " enable_follow: " << p.enable_follow << ',' << " flavor: " << p.flavor
      << ',' << " follow_which: " << p.follow_which << ',' << " bx: " << p.bx
      << ',' << " orbit_prescale: " << p.orbit_prescale << ','
      << " burst_length: " << p.burst_length << " }";
    return o;
  }
  void pack() {
    uint32_t a(0);
    if (enable) a |= 0x1;
    if (enable_follow) a |= 0x4;
    a |= (flavor & 0x7) << 3;
    a |= (follow_which & 0xF) << 20;
    a |= (bx & 0xFFF) << 8;
    uio_.write(offset_, a);
    uint32_t b(orbit_prescale & 0xFFFFF);
    b |= (burst_length & 0x3FF) << 20;
    uio_.write(offset_ + 1, b);
  }

 private:
  UIO& uio_;
  size_t offset_;
};

class FastControlCMS_MMap : public FastControl {
 public:
  FastControlCMS_MMap() : FastControl(), uio_("fastcontrol_axi", 4096) {
    pflib_log(debug) << "pedestal fast command: "
                     << periodic(PEDESTAL_PERIODIC);
    pflib_log(debug) << "charge fast command: " << periodic(CHARGE_PERIODIC);
    pflib_log(debug) << "charge-l1a fast command: "
                     << periodic(CHARGE_L1A_PERIODIC);
    pflib_log(debug) << "led fast command: " << periodic(LED_PERIODIC);
    pflib_log(debug) << "led-l1a fast command: " << periodic(LED_L1A_PERIODIC);
  }

  ~FastControlCMS_MMap() = default;

  Periodic periodic(int i) { return Periodic(uio_, 0x20 + i * 2); }

  static const int PEDESTAL_PERIODIC = 2;
  static const int CHARGE_PERIODIC = 3;
  static const int CHARGE_L1A_PERIODIC = 4;
  static const int LED_PERIODIC = 5;
  static const int LED_L1A_PERIODIC = 6;

  void standard_setup() override {
    Periodic std_l1a(periodic(PEDESTAL_PERIODIC));
    std_l1a.bx = 10;
    std_l1a.flavor = 0;
    std_l1a.orbit_prescale = 1000;
    std_l1a.enable_follow = false;
    std_l1a.enable = false;
    std_l1a.pack();

    Periodic charge_inj(periodic(CHARGE_PERIODIC));
    charge_inj.bx = 30;     // needs tuning
    charge_inj.flavor = 2;  // internal calibration pulse
    charge_inj.enable_follow = false;
    charge_inj.orbit_prescale = 1000;
    charge_inj.enable = false;
    charge_inj.pack();

    Periodic l1a_charge(periodic(CHARGE_L1A_PERIODIC));
    // for a DIGITALHALF_{0,1}.L1OFFSET = 16 (the chip default)
    // charge injection pulses were observed at a separation of 20
    l1a_charge.bx = charge_inj.bx + 20;
    l1a_charge.flavor = 0;
    l1a_charge.enable_follow = true;
    l1a_charge.follow_which = CHARGE_PERIODIC;
    l1a_charge.orbit_prescale = 0;
    l1a_charge.enable = true;
    l1a_charge.pack();

    Periodic pled(periodic(LED_PERIODIC));
    pled.bx = 12;     // needs tuning
    pled.flavor = 3;  // external calibration pulse
    pled.enable_follow = false;
    pled.orbit_prescale = 1000;
    pled.enable = false;
    pled.pack();

    Periodic l1a_led(periodic(LED_L1A_PERIODIC));
    l1a_led.bx = 30;  // needs tuning
    l1a_led.flavor = 0;
    l1a_led.enable_follow = true;
    l1a_led.follow_which = LED_PERIODIC;
    l1a_led.orbit_prescale = 0;
    l1a_led.enable = true;
    l1a_led.pack();

    // enable the BCR, L1As (in general)
    uio_.rmw(ADDR_CTL_REG, CTL_ENABLE_ORBITSYNC, CTL_ENABLE_ORBITSYNC);
    uio_.rmw(ADDR_CTL_REG, CTL_ENABLE_L1AS, CTL_ENABLE_L1AS);
  }

  virtual void resetCounters() override {
    uio_.rmw(ADDR_REQUEST, REQ_CLEAR_COUNTERS, REQ_CLEAR_COUNTERS);
  }

  virtual int fc_get_setup_calib() override {
    Periodic charge_inj(periodic(CHARGE_PERIODIC));
    Periodic l1a_charge(periodic(CHARGE_L1A_PERIODIC));
    return l1a_charge.bx - charge_inj.bx;
  }

  virtual void fc_setup_calib(int charge_to_l1a) override {
    Periodic charge_inj(periodic(CHARGE_PERIODIC));
    Periodic l1a_charge(periodic(CHARGE_L1A_PERIODIC));
    l1a_charge.bx = charge_inj.bx + charge_to_l1a;
    l1a_charge.pack();
  }

  virtual int fc_get_setup_led() override {
    Periodic led_flash(periodic(LED_PERIODIC));
    Periodic l1a_led(periodic(LED_L1A_PERIODIC));
    return l1a_led.bx - led_flash.bx;
  }

  virtual void fc_setup_led(int led_to_l1a) override {
    Periodic led_flash(periodic(LED_PERIODIC));
    Periodic l1a_led(periodic(LED_L1A_PERIODIC));
    l1a_led.bx = led_flash.bx + led_to_l1a;
    l1a_led.pack();
  }

  virtual void sendL1A() override { periodic(PEDESTAL_PERIODIC).request(); }

  virtual std::vector<uint32_t> getCmdCounters() override {
    std::vector<uint32_t> retval;
    for (int i = 65; i <= 89; i++) retval.push_back(uio_.read(i));
    return retval;
  }

  static const uint32_t REQ_reset_nzs =
      0x1;  // Reset the NZS generator (auto-clear)/>
  static const uint32_t REQ_count_rst = 0x2;  // Reset counters (auto-clear)/>
  static const uint32_t REQ_sequence_req =
      0x8000;  // Request a single operation of the sequencer block/>
  static const uint32_t REQ_orbit_count_reset =
      0x10000;  // Send an orbit count reset at the next orbitsync
                // (auto-clear)/>
  static const uint32_t REQ_chipsync =
      0x20000;  // Send a ChipSync (auto-clear)/>
  static const uint32_t REQ_ebr =
      0x40000;  // Send an EventBufferReset (ebr) (auto-clear)/>
  static const uint32_t REQ_ecr =
      0x80000;  // Send an EventCounterReset (ecr) (auto-clear)/>
  static const uint32_t REQ_link_reset_roct =
      0x100000;  // Send a link-reset_ROC_T (auto-clear)/>
  static const uint32_t REQ_link_reset_rocd =
      0x200000;  // Send a link-reset_ROC_D (auto-clear)/>
  static const uint32_t REQ_link_reset_econt =
      0x400000;  // Send a link-reset_ECON_T (auto-clear)/>
  static const uint32_t REQ_link_reset_econd =
      0x800000;  // Send a link-reset_ECON_D (auto-clear)/>
  static const uint32_t REQ_spare0 =
      0x1000000;  // Send a SPARE0 command (auto-clear)/>
  static const uint32_t REQ_spare1 =
      0x2000000;  // Send a SPARE1 command (auto-clear)/>
  static const uint32_t REQ_spare2 =
      0x4000000;  // Send a SPARE2 command (auto-clear)/>
  static const uint32_t REQ_spare3 =
      0x8000000;  // Send a SPARE3 command (auto-clear)/>
  static const uint32_t REQ_spare4 =
      0x10000000;  // Send a SPARE4 command (auto-clear)/>
  static const uint32_t REQ_spare5 =
      0x20000000;  // Send a SPARE5 command (auto-clear)/>
  static const uint32_t REQ_spare6 =
      0x40000000;  // Send a SPARE6 command (auto-clear)/>
  static const uint32_t REQ_spare7 =
      0x80000000u;  // Send a SPARE7 command (auto-clear)/>

  void bx_custom(int bx_addr, int bx_mask, int bx_new) {
    // uio_.rmw(bx_addr, bx_mask, bx_mask);
    // turn off L1A for the moment
    uint32_t preval = uio_.read(ADDR_CTL_REG);
    uio_.write(ADDR_CTL_REG, ((preval | CTL_ENABLE_L1AS) ^ CTL_ENABLE_L1AS));

    uint32_t bx_out = uio_.readMasked(bx_addr, bx_mask);
    std::cout << "readMasked: " << bx_out << std::endl;
    // uint32_t bx_out_write = uio_.writeMasked(bx_addr, bx_mask, bx_new);
    std::cout << "readMasked (after write): " << bx_out << std::endl;

    // restore previous L1A situation
    uio_.write(ADDR_CTL_REG, preval);
    // uio_.rmw(bx_addr, bx_mask, bx_new);
  }

  virtual void linkreset_rocs() override {
    // turn off L1A for the moment
    uint32_t preval = uio_.read(ADDR_CTL_REG);
    uio_.write(ADDR_CTL_REG, ((preval | CTL_ENABLE_L1AS) ^ CTL_ENABLE_L1AS));

    uio_.rmw(ADDR_REQUEST, REQ_link_reset_roct, REQ_link_reset_roct);
    usleep(10);
    uio_.rmw(ADDR_REQUEST, REQ_link_reset_rocd, REQ_link_reset_rocd);

    // restore previous L1A situation
    uio_.write(ADDR_CTL_REG, preval);
  }

  virtual void orbit_count_reset() override {
    // turn off L1A for the moment
    uint32_t preval = uio_.read(ADDR_CTL_REG);
    uio_.write(ADDR_CTL_REG, ((preval | CTL_ENABLE_L1AS) ^ CTL_ENABLE_L1AS));

    uio_.rmw(ADDR_REQUEST, REQ_orbit_count_reset, REQ_orbit_count_reset);

    // restore previous L1A situation
    uio_.write(ADDR_CTL_REG, preval);
  }

  virtual void clear_run() override {
    // turn off L1A for the moment
    uint32_t preval = uio_.read(ADDR_CTL_REG);
    uio_.write(ADDR_CTL_REG, ((preval | CTL_ENABLE_L1AS) ^ CTL_ENABLE_L1AS));

    uio_.rmw(ADDR_REQUEST, REQ_ecr, REQ_ecr);

    // restore previous L1A situation
    uio_.write(ADDR_CTL_REG, preval);
  }

  virtual void bufferclear() override {
    // turn off L1A for the moment
    uint32_t preval = uio_.read(ADDR_CTL_REG);
    uio_.write(ADDR_CTL_REG, ((preval | CTL_ENABLE_L1AS) ^ CTL_ENABLE_L1AS));

    uio_.rmw(ADDR_REQUEST, REQ_ebr, REQ_ebr);

    // restore previous L1A situation
    uio_.write(ADDR_CTL_REG, preval);
  }

  virtual void chargepulse() override { periodic(CHARGE_PERIODIC).request(); }

  virtual void ledpulse() override { periodic(LED_PERIODIC).request(); }

 private:
  UIO uio_;
  mutable logging::logger the_log_{logging::get("FastControlCMS_MMap")};
};

FastControl* make_FastControlCMS_MMap() { return new FastControlCMS_MMap(); }

}  // namespace pflib
