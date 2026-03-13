#pragma once
#include "../pftool.h"

/**
 * Calls the delay_scan algorithm to find the optimal delay settings for the ADC bits using INL-minimization.
*/

void set_delays(Target* tgt);