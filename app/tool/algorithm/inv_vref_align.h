#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace pflib {
class Target;
class ROC;
} 

namespace pflib::algorithm {

struct InvVrefScanResult {
  std::vector<int> inv_vref;

  int ch0{-1};
  int ch1{-1};

  std::vector<int> adc0;
  std::vector<int> adc1;
};

/**
 * Run the INV_VREF scan in-memory (no CSV output).
 * Mirrors tasks.inv_vref_scan: inv_vref = 0..600 step 20, trigger=PEDESTAL.
 */
InvVrefScanResult inv_vref_scan_data(pflib::Target* tgt, pflib::ROC& roc,
                                     int nevents = 100, int ch0 = 17,
                                     int ch1 = 51, int step = 20,
                                     int max_vref = 600);

/**
 * Compute optimal INV_VREF per link from scan data using the same logic as
 * inv_vref_align.py (linear regression in the 10%..80% ADC range).
 */
std::map<std::string, std::map<std::string, uint64_t>> inv_vref_align(
    const InvVrefScanResult& scan, int target_adc = 100);

}  // namespace pflib::algorithm
