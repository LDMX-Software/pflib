#ifndef PFLIB_TARGET_H_INCLUDED
#define PFLIB_TARGET_H_INCLUDED

#include "pflib/FastControl.h"
#include "pflib/Hcal.h"
#include "pflib/I2C.h"

namespace pflib {

/**
 * @class Target class for encapulating a given setup's access rules
 */
class Target {
 public:
  ~Target() {}

  std::vector<std::string> i2c_bus_names();
  I2C& get_i2c_bus(const std::string& name);

  Hcal& hcal() { return *hcal_; }

  FastControl& fc() { return *fc_; }

  virtual void setup_run(int irun, int format, int contrib_id = -1) {}
  virtual std::vector<uint32_t> read_event() = 0;
  virtual bool has_event() { return hcal().daq().getEventOccupancy() > 0; }

  /**
   * Abstract base class for consuming event packets
   */
  class DAQRunConsumer {
   public:
    virtual void consume(std::vector<uint32_t>& event) = 0;
    virtual ~DAQRunConsumer() = default;
  };

  /**
   * Do a DAQ run
   * 
   * @param[in] cmd PEDESTAL, CHARGE, or LED (type of trigger to send)
   * @param[in] consumer DAQRunConsumer that handles the readout event packets
   * (probably writes them to a file or something like that)
   * @param[in] nevents number of events to collect
   * @param[in] rate how fast to collect events, default 100
   */
  void daq_run(
      const std::string& cmd,
      DAQRunConsumer& consumer,
      int nevents,
      int rate = 100
  );

 protected:
  std::map<std::string, std::shared_ptr<I2C> > i2c_;

  std::shared_ptr<Hcal> hcal_;
  std::shared_ptr<FastControl> fc_;
  mutable logging::logger the_log_{logging::get("Target")};
};

Target* makeTargetFiberless();

}  // namespace pflib

#endif  // PFLIB_TARGET_H_INCLUDED
