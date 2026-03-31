#pragma once

#include "../pftool.h"
#include "pflib/Target.h"

/**
 * The scan goes through a range of CALIB values in preCC, allowing for INL and DNL calculation by tracking the peaks of the scans.
 * The output is an array of different maximum values: 
 * INL calculated through a linear fit of the peaks, INL calculated by comparison of the peaks with an extrapolated "ideal" INL, and the DNL.
 * Later on, the scan will be expanded to save all INL/DNL values, as well as the data necessary for plotting the INL/DNL as sanity-checks.
 */
namespace pflib::algorithm {

  template <class EventPacket>
  std::vector<double> nl_scan(Target* tgt, ROC& roc, size_t& n_events, int& channel, int& link, std::array<int,4> delays, std::vector<double> CALIBs, double& optimal_bx);

}
