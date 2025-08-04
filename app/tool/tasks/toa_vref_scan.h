#pragma once

#include "../pftool.h"

/**
 * TASKS.TOA_VREF_SCAN
 *
 * Scan TOA_VREF for both halves of the chip
 * Check ADC Pedestals for each point
 * Used to align TOA trigger to fire just above pedestals
 * Should be done after pedestal alignment
 */
void toa_vref_scan(Target* tgt);
