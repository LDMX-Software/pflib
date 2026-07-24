/**
   This version of the fast control code interfaces with the CMS
   Fast control library which can be controlled over MMap/UIO
*/
#include <stdint.h>
#include <stdio.h>

#include <ostream>

#include "pflib/Exception.h"
#include "pflib/FastControl.h"
#include "pflib/logging/Logging.h"
#include "pflib/zcu/UIO.h"

namespace pflib {

static const size_t ADDR_CTL_REG = 0;
static const size_t ADDR_REQUEST = 1;
static const size_t ADDR_EXT_TRIG_BURST_LEN = 0xa;

static const uint32_t REQ_CLEAR_COUNTERS = 0x2;
static const uint32_t CTL_ENABLE_ORBITSYNC = 0x0004;
static const uint32_t CTL_ENABLE_L1AS = 0x0008;
static const uint32_t CTL_ENABLE_EXT_L1A = 0x0080;
static const uint32_t MASK_EXT_TRIG_BURST_LEN = 0x3ff0000;

/*
 * see address map listed in hgcal_fc_manager.v
 * and documented in the sw xml
 * https://github.com/slaclab/ldmx-firmware/blob/hcal_zcu102/firmware/targets/LpGBTZCU/ip_repo/fast-control/sw/xml/fastcontrol_axi.xml
 *
 * of note:
 *
 * addr | bits  | description
 *    0 |     0 | enable fast control stream (otherwise constant zero)
 *    0 |     3 | global enable/disable of L1As
 *    0 | 10: 7 | enable external L1A sources 0-3
 *    0 |  5: 4 | pre-L1A offset
 *    a | 25:16 | number of consecutive L1As to send for a each external trigger
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
    pflib_log(debug) << "pedestal fast command: " << periodic(PEDESTAL);
    pflib_log(debug) << "charge fast command: " << periodic(CHARGE);
    pflib_log(debug) << "charge-l1a fast command: " << periodic(CHARGE_ROR);
    pflib_log(debug) << "led fast command: " << periodic(LED);
    pflib_log(debug) << "led-l1a fast command: " << periodic(LED_ROR);
    pflib_log(debug) << "orbit blinker fast command: "
                     << periodic(ORBIT_BLINKER);
    pflib_log(debug) << "single L1A fast command: " << periodic(SINGLE_L1A);
  }

  ~FastControlCMS_MMap() = default;

  Periodic periodic(int i) { return Periodic(uio_, 0x20 + i * 2); }

  static const int ORBIT_BLINKER = 1;
  static const int PEDESTAL = 2;
  static const int CHARGE = 3;
  static const int CHARGE_ROR = 4;
  static const int LED = 5;
  static const int LED_ROR = 6;
  static const int SINGLE_L1A = 7;

  void standard_setup() override {
    Periodic orbit_blinker(periodic(ORBIT_BLINKER));
    orbit_blinker.bx = 10;
    orbit_blinker.flavor = 0;
    orbit_blinker.orbit_prescale = 0;
    orbit_blinker.enable_follow = false;
    orbit_blinker.enable = false;
    orbit_blinker.pack();

    Periodic std_l1a(periodic(SINGLE_L1A));
    std_l1a.bx = 10;
    std_l1a.flavor = 0;
    std_l1a.orbit_prescale = 1000;
    std_l1a.enable_follow = false;
    std_l1a.enable = false;
    std_l1a.pack();

    Periodic pedestal(periodic(PEDESTAL));
    pedestal.bx = 10;
    pedestal.flavor = 0;
    pedestal.orbit_prescale = 1000;
    pedestal.enable_follow = false;
    pedestal.enable = false;
    pedestal.pack();

    Periodic charge_inj(periodic(CHARGE));
    charge_inj.bx = 30;     // needs tuning
    charge_inj.flavor = 2;  // internal calibration pulse
    charge_inj.enable_follow = false;
    charge_inj.orbit_prescale = 1000;
    charge_inj.enable = false;
    charge_inj.pack();

    Periodic ror_charge(periodic(CHARGE_ROR));
    // for a DIGITALHALF_{0,1}.L1OFFSET = 8 (the chip default)
    // charge injection pulses were observed at a separation of 20
    ror_charge.bx = charge_inj.bx + 20;
    ror_charge.flavor = 0;
    ror_charge.enable_follow = true;
    ror_charge.follow_which = CHARGE;
    ror_charge.orbit_prescale = 0;
    ror_charge.enable = true;
    ror_charge.pack();

    Periodic led(periodic(LED));
    led.bx = 12;     // needs tuning
    led.flavor = 3;  // external calibration pulse
    led.enable_follow = false;
    led.orbit_prescale = 1000;
    led.enable = false;
    led.pack();

    Periodic ror_led(periodic(LED_ROR));
    ror_led.bx = 30;  // needs tuning
    ror_led.flavor = 0;
    ror_led.enable_follow = true;
    ror_led.follow_which = LED;
    ror_led.orbit_prescale = 0;
    ror_led.enable = true;
    ror_led.pack();

    // enable the BCR, L1As (in general)
    uio_.rmw(ADDR_CTL_REG, CTL_ENABLE_ORBITSYNC, CTL_ENABLE_ORBITSYNC);
    uio_.rmw(ADDR_CTL_REG, CTL_ENABLE_L1AS, CTL_ENABLE_L1AS);
  }

  void fc_enables_read(bool& overall, bool& external) override {
    uint32_t ctl_reg{uio_.read(ADDR_CTL_REG)};
    overall = (ctl_reg & CTL_ENABLE_L1AS);
    external = ((ctl_reg >> 7) & 0x1);
    return;
  }

  void fc_enables(bool overall, bool external) override {
    uio_.rmw(ADDR_CTL_REG, CTL_ENABLE_L1AS, overall ? CTL_ENABLE_L1AS : 0);
    uio_.writeMasked(ADDR_CTL_REG, CTL_ENABLE_EXT_L1A, external);
  }

  virtual void resetCounters() override {
    uio_.rmw(ADDR_REQUEST, REQ_CLEAR_COUNTERS, REQ_CLEAR_COUNTERS);
  }

  virtual void fc_setup_orbit_blinker(bool enable, int bx) override {
    Periodic orbit_blinker(periodic(ORBIT_BLINKER));
    orbit_blinker.bx = bx;
    orbit_blinker.enable = enable;
    orbit_blinker.pack();
  }

  virtual void fc_get_orbit_blinker(bool& enable, int& bx) override {
    Periodic orbit_blinker(periodic(ORBIT_BLINKER));
    bx = orbit_blinker.bx;
    enable = orbit_blinker.enable;
  }

  virtual void fc_get_setup_calib(int& charge_to_l1a,
                                  bool& enable_follow_l1a) override {
    Periodic charge_inj(periodic(CHARGE));
    Periodic ror_charge(periodic(CHARGE_ROR));
    charge_to_l1a = ror_charge.bx - charge_inj.bx;
    enable_follow_l1a = ror_charge.enable_follow and ror_charge.enable;
  }

  virtual void fc_setup_calib(int charge_to_l1a,
                              bool enable_follow_l1a) override {
    Periodic charge_inj(periodic(CHARGE));
    Periodic ror_charge(periodic(CHARGE_ROR));
    ror_charge.bx = charge_inj.bx + charge_to_l1a;
    ror_charge.enable_follow = enable_follow_l1a;
    ror_charge.enable = enable_follow_l1a;
    ror_charge.pack();
  }

  virtual int fc_get_setup_led() override {
    Periodic led_flash(periodic(LED));
    Periodic ror_led(periodic(LED_ROR));
    return ror_led.bx - led_flash.bx;
  }

  virtual void fc_setup_led(int led_to_l1a) override {
    Periodic led_flash(periodic(LED));
    Periodic ror_led(periodic(LED_ROR));
    ror_led.bx = led_flash.bx + led_to_l1a;
    ror_led.pack();
  }

  virtual std::map<std::string, uint32_t> getCmdCounters() override {
    static constexpr int COUNTER_START = 68;
    static constexpr int COUNTER_LAST = 80;
    static constexpr const char* names[] = {"L1A",
                                            "L1A_NZS",
                                            "ORBIT_SYNC",
                                            "ORBIT_COUNT_RESET",
                                            "CALIB_INT",
                                            "CALIB_EXT",
                                            "CHIPSYNC",
                                            "ECR",
                                            "EBR",
                                            "LINKRESET_ROCT",
                                            "LINKRESET_ROCD",
                                            "LINKRESET_ECONT",
                                            "LINKRESET_ECOND",
                                            0};
    std::map<std::string, uint32_t> retval;
    for (int i = COUNTER_START; i <= COUNTER_LAST; i++)
      retval[names[i - COUNTER_START]] = uio_.read(i);
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

  virtual void linkreset_econs() override {
    // turn off L1A for the moment
    uint32_t preval = uio_.read(ADDR_CTL_REG);
    uio_.write(ADDR_CTL_REG, ((preval | CTL_ENABLE_L1AS) ^ CTL_ENABLE_L1AS));

    uio_.rmw(ADDR_REQUEST, REQ_link_reset_econd, REQ_link_reset_econd);
    usleep(1000);
    uio_.rmw(ADDR_REQUEST, REQ_link_reset_econt, REQ_link_reset_econt);
    // restore previous situation
    uio_.write(ADDR_CTL_REG, preval);
  }

  virtual void clear_run() override {
    // turn off L1A for the moment
    uint32_t preval = uio_.read(ADDR_CTL_REG);
    uio_.write(ADDR_CTL_REG, ((preval | CTL_ENABLE_L1AS) ^ CTL_ENABLE_L1AS));

    resetCounters();
    usleep(1000);
    bufferclear();
    usleep(1000);
    orbit_count_reset();
    usleep(1000);
    uio_.rmw(ADDR_REQUEST, REQ_ecr, REQ_ecr);
    usleep(1000);

    // restore previous situation
    uio_.write(ADDR_CTL_REG, preval);
  }

  virtual void bufferclear() override {
    // turn off L1A for the moment
    uint32_t preval = uio_.read(ADDR_CTL_REG);
    uio_.write(ADDR_CTL_REG, ((preval | CTL_ENABLE_L1AS) ^ CTL_ENABLE_L1AS));

    uio_.rmw(ADDR_REQUEST, REQ_ebr, REQ_ebr);
    // restore previous situation
    uio_.write(ADDR_CTL_REG, preval);
  }

  virtual void sendL1A() override { periodic(SINGLE_L1A).request(); }
  virtual void chargepulse() override { periodic(CHARGE).request(); }
  virtual void ledpulse() override { periodic(LED).request(); }
  virtual void sendROR() override { periodic(PEDESTAL).request(); }
  void setL1AperROR(int n) override {
    if (n < 1) {
      PFEXCEPTION_RAISE(
          "MalForm", "It doesn't make sence to have less than 1 L1A per RoR.");
    }
    auto pedestal(periodic(PEDESTAL));
    pedestal.burst_length = n;
    pedestal.pack();
    auto charge_ror(periodic(CHARGE_ROR));
    charge_ror.burst_length = n;
    charge_ror.pack();
    auto led_ror{periodic(LED_ROR)};
    led_ror.burst_length = n;
    led_ror.pack();
    uio_.writeMasked(ADDR_EXT_TRIG_BURST_LEN, MASK_EXT_TRIG_BURST_LEN, n);
  }
  int getL1AperROR() override {
    // assume the three ROR commands (PEDESTAL, CHARGE_ROR, and LED_ROR)
    // are all matching
    int length = periodic(PEDESTAL).burst_length;
    if (length == 0) return 1;
    return length;
  }

 private:
  UIO uio_;
  bool enable_charge_follow = true;
  mutable logging::logger the_log_{logging::get("FastControlCMS_MMap")};
};

FastControl* make_FastControlCMS_MMap() { return new FastControlCMS_MMap(); }

}  // namespace pflib
