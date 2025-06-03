#include "pflib/Target.h"

#include <sys/time.h>

namespace pflib {

std::vector<std::string> Target::i2c_bus_names() {
  std::vector<std::string> names;
  for (auto items : i2c_) names.push_back(items.first);
  return names;
}

I2C& Target::get_i2c_bus(const std::string& name) { return *(i2c_[name]); }

void Target::daq_run(
    const std::string& cmd,
    DAQRunConsumer& consumer,
    int nevents,
    int rate
) {
  static const
  std::unordered_map<std::string, std::function<void(pflib::FastControl&)>>
  cmds = {
    {"PEDESTAL", [](pflib::FastControl& fc) { fc.sendL1A(); }},
    {"CHARGE"  , [](pflib::FastControl& fc) { fc.chargepulse(); }},
    {"LED"     , [](pflib::FastControl& fc) { fc.ledpulse(); }}
  };
  if (cmds.find(cmd) == cmds.end()) {
    PFEXCEPTION_RAISE("UnknownCMD", "Command "+cmd+" is not one of the daq_run options.");
  }
  auto trigger{cmds.at(cmd)};

  timeval tv0, tvi;
  gettimeofday(&tv0, 0);
  for (int ievt = 0; ievt < nevents; ievt++) {
    pflib_log(trace) << "daq event occupancy pre-L1A    : "
                     << this->hcal().daq().getEventOccupancy();
    trigger(this->fc());

    pflib_log(trace) << "daq event occupancy post-L1A   : "
                     << this->hcal().daq().getEventOccupancy();
    gettimeofday(&tvi, 0);
    double runsec =
        (tvi.tv_sec - tv0.tv_sec) + (tvi.tv_usec - tv0.tv_usec) / 1e6;
    double targettime = (ievt + 1.0) / rate;
    int usec_ahead = int((targettime - runsec) * 1e6);
    pflib_log(trace) << " at " << runsec << "s instead of " << targettime
                     << "s aheady by " << usec_ahead << "us";
    if (usec_ahead > 100) {
      usleep(usec_ahead);
    }

    pflib_log(trace) << "daq event occupancy after pause: "
                     << this->hcal().daq().getEventOccupancy();

    std::vector<uint32_t> event = this->read_event();
    pflib_log(trace) << "daq event occupancy after read : "
                     << this->hcal().daq().getEventOccupancy();
    pflib_log(debug) << "event " << ievt << " has " << event.size()
                     << " 32-bit words";
    if (event.size() == 0) {
      pflib_log(warn) << "event " << ievt
                      << " did not have any words, skipping.";
    } else {
      consumer.consume(event);
    }
  }
}

}  // namespace pflib
