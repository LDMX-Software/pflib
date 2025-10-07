#pragma once

#include "../pftool.h"

/**
 * TASKS.TRIM_TOA_SCAN
 *
 * Scan TRIM_TOA  and CALIB for each channel on chip
 * Used to align TOA measurement after TOA_VREF scan
 */
void trim_toa_scan(Target* tgt);
