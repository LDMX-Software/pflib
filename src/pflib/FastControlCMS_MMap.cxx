/**
   This version of the fast control code interfaces with the CMS
   Fast control library which can be controlled over MMap/UIO
*/
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include "pflib/FastControl.h"
#include "pflib/Exception.h"

namespace pflib {

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

class Periodic {
 public:
  bool enable;
  int flavor;
  bool enable_follow;
  int follow_which;
  int bx;
  int orbit_prescale;
  int burst_length;

  Periodic(uint32_t* image) : _image{image} { reload(); }
  void reload() {
    uint32_t a=_image[0]; // get into a register
    enable=a&0x1;
    enable_follow=a&0x4;
    flavor=(a&0x38)>>3;
    follow_which=(a&0xF00000)>>20;
    bx=(a&0xFFF00)>>8;
    orbit_prescale=(_image[1]&0xFFFFF);
    burst_length=(_image[1]>>20)&0x3FF;
  }
  void request() {
    _image[0]|=0x2; 
  }
  void pack() {
    uint32_t a(0);
    if (enable) a|=0x1;
    if (enable_follow) a|=0x4;
    a|=(flavor&0x7)<<3;
    a|=(follow_which*0xF)<<20;
    a|=(bx&0xFFF)<<8;
    //    _image[0]=a;
    uint32_t b(orbit_prescale&0xFFFFF);
    b|=(burst_length&0x3FF)<<20;
    //  _image[1]=b;
  }
 private:
  uint32_t* _image;
};

static const int MAP_SIZE=4096;

class FastControlCMS_MMap : public FastControl {
 public:
  FastControlCMS_MMap() : FastControl()  {
    handle_=open("/dev/uio4",O_RDWR);
    if (handle_<0) {
      char msg[100];
      snprintf(msg,100,"Error opening /dev/uio4 : %d", errno);
      PFEXCEPTION_RAISE("DeviceFileAccessError",msg);
    }
    base_=(uint32_t*)(mmap(0,MAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,handle_,0));
    if (base_==MAP_FAILED) {
      PFEXCEPTION_RAISE("DeviceFileAccessError","Failed to mmap FastControl memory block");
    }
    standard_setup();
  }
  
  ~FastControlCMS_MMap() {
    if (handle_!=0) {
      munmap(base_,MAP_SIZE);
    }
  }

  Periodic periodic(int i) { return Periodic(base_+0x20+i*2); }

  static const int PEDESTAL_PERIODIC=2;
  static const int CHARGE_PERIODIC=3;
  static const int CHARGE_L1A_PERIODIC=4;
  static const int LED_PERIODIC=5;
  static const int LED_L1A_PERIODIC=6;
  
  void standard_setup() {
    Periodic std_l1a(periodic(PEDESTAL_PERIODIC));

    std_l1a.bx=10;
    std_l1a.flavor=0;
    std_l1a.orbit_prescale=1000;
    std_l1a.enable_follow=false;
    std_l1a.enable=false;
    std_l1a.pack();

    Periodic charge_inj(periodic(CHARGE_PERIODIC));
    charge_inj.bx=30; // needs tuning
    charge_inj.flavor=2;  // internal calibration pulse
    charge_inj.enable_follow=false;
    charge_inj.orbit_prescale=1000;
    charge_inj.enable=false;
    charge_inj.pack();

    Periodic l1a_charge(periodic(CHARGE_L1A_PERIODIC));
    l1a_charge.bx=90; // needs tuning
    l1a_charge.flavor=0;
    l1a_charge.enable_follow=true;
    l1a_charge.follow_which=CHARGE_PERIODIC;
    l1a_charge.orbit_prescale=0;
    l1a_charge.enable=true;
    l1a_charge.pack();

    Periodic pled(periodic(LED_PERIODIC));
    pled.bx=30; // needs tuning
    pled.flavor=3;  // external calibration pulse
    pled.enable_follow=false;
    pled.orbit_prescale=1000;
    pled.enable=false;
    pled.pack();

    Periodic l1a_led(periodic(LED_L1A_PERIODIC));
    l1a_led.bx=100; // needs tuning
    l1a_led.flavor=0;
    l1a_led.enable_follow=true;
    l1a_led.follow_which=LED_PERIODIC;
    l1a_led.orbit_prescale=0;
    l1a_led.enable=true;
    l1a_led.pack();
    
  }

  virtual void sendL1A() {
    periodic(PEDESTAL_PERIODIC).request();
  }

  virtual std::vector<uint32_t> getCmdCounters() {
    std::vector<uint32_t> retval;
    for (int i=65; i<=89; i++)
      retval.push_back(base_[i]);
    return retval;
  }
  
  static const uint32_t REQ_reset_nzs               = 0x1;         // Reset the NZS generator (auto-clear)/>
  static const uint32_t REQ_count_rst               = 0x2;         // Reset counters (auto-clear)/>
  static const uint32_t REQ_sequence_req            = 0x8000;      // Request a single operation of the sequencer block/>
  static const uint32_t REQ_orbit_count_reset       = 0x10000;     // Send an orbit count reset at the next orbitsync (auto-clear)/>
  static const uint32_t REQ_chipsync                = 0x20000;     // Send a ChipSync (auto-clear)/>
  static const uint32_t REQ_ebr                     = 0x40000;     // Send an EventBufferReset (ebr) (auto-clear)/>
  static const uint32_t REQ_ecr                     = 0x80000;     // Send an EventCounterReset (ecr) (auto-clear)/>
  static const uint32_t REQ_link_reset_roct         = 0x100000;    // Send a link-reset_ROC_T (auto-clear)/>
  static const uint32_t REQ_link_reset_rocd         = 0x200000;    // Send a link-reset_ROC_D (auto-clear)/>
  static const uint32_t REQ_link_reset_econt        = 0x400000;    // Send a link-reset_ECON_T (auto-clear)/>
  static const uint32_t REQ_link_reset_econd        = 0x800000;    // Send a link-reset_ECON_D (auto-clear)/>
  static const uint32_t REQ_spare0                  = 0x1000000;   // Send a SPARE0 command (auto-clear)/>
  static const uint32_t REQ_spare1                  = 0x2000000;   // Send a SPARE1 command (auto-clear)/>
  static const uint32_t REQ_spare2                  = 0x4000000;   // Send a SPARE2 command (auto-clear)/>
  static const uint32_t REQ_spare3                  = 0x8000000;   // Send a SPARE3 command (auto-clear)/>
  static const uint32_t REQ_spare4                  = 0x10000000;  // Send a SPARE4 command (auto-clear)/>
  static const uint32_t REQ_spare5                  = 0x20000000;// Send a SPARE5 command (auto-clear)/>
  static const uint32_t REQ_spare6                  = 0x40000000;  // Send a SPARE6 command (auto-clear)/>
  static const uint32_t REQ_spare7                  = 0x80000000u;  // Send a SPARE7 command (auto-clear)/>
  
   virtual void linkreset_rocs() {
    FastControlAXI* axi=(FastControlAXI*)base_;
    axi->request|=REQ_link_reset_roct;
    // must have a break here
    axi->request|=REQ_link_reset_rocd;
  }

  virtual void bufferclear() {
    FastControlAXI* axi=(FastControlAXI*)base_;
    axi->request|=REQ_ebr;
  }

  virtual void chargepulse() {
    periodic(CHARGE_PERIODIC).request();
  }

  virtual void ledpulse() {
    periodic(LED_PERIODIC).request();
  }

  
 private:
  int handle_;
  uint32_t* base_;
};

FastControl* make_FastControlCMS_MMap() { return new FastControlCMS_MMap(); }

}
