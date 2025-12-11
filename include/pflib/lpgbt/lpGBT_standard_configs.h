#ifndef lpgbt_standard_configs_h_included
#define lpgbt_standard_configs_h_included 1

#include "pflib/lpGBT.h"

namespace pflib {
namespace lpgbt {
namespace standard_config {

/** Setup the standard set of I/Os for the HCAL backplane DAQ lpGBT */
void setup_hcal_daq(pflib::lpGBT&);
/** Setup the GPIO names for the HCAL backplane DAQ lpGBT */
void setup_hcal_daq_gpio(pflib::lpGBT&);
void setup_hcal_trig_gpio(pflib::lpGBT&);

/** Setup the standard set of I/Os for the HCAL backplane TRIG lpGBT */
void setup_hcal_trig(pflib::lpGBT&);
/** Setup the GPIO names for the ECAL DAQ*/
void setup_ecal_daq_gpio(pflib::lpGBT&);

/** Setup the lpGBT to train the eRx phase */
void setup_erxtraining(pflib::lpGBT&, bool prbs_on);

enum class ECAL_lpGBT_Config {
  DAQ_SingleModuleMotherboard = 101,
  TRIG_SingleModuleMotherboard = 201,
};

/** Setup the standard set of I/Os for ECAL, depending on the use case */
void setup_ecal(pflib::lpGBT&, ECAL_lpGBT_Config);

}  // namespace standard_config
}  // namespace lpgbt
}  // namespace pflib

#endif  // lpgbt_standard_configs_h_included
