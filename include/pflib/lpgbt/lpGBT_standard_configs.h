#ifndef lpgbt_standard_configs_h_included
#define lpgbt_standard_configs_h_included 1

#include "pflib/lpGBT.h"

namespace pflib {
namespace lpgbt {
namespace standard_config {

/** Setup the standard set of I/Os for the HCAL backplane DAQ lpGBT */
void setup_hcal_daq(pflib::lpGBT&);

/** Setup the standard set of I/Os for the HCAL backplane TRIG lpGBT */
void setup_hcal_trig(pflib::lpGBT&);

}  // namespace standard_config
}  // namespace lpgbt
}  // namespace pflib

#endif  // lpgbt_standard_configs_h_included
