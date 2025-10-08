#pragma once
#include "../pftool.h"

/**
 * TASKS.TOA_VREF_SCAN
 *
 * Scan TOA_VREF for both halves of the chip
 * Check ADC Pedestals for each point
 * Used to align TOA trigger to fire just above pedestals
 * Should be done after global pedestal alignment
 *
 * TODO: this scan will also output the ideal TOA_VREF
 * values using the pedestals method.
 */
void toa_vref_scan(Target* tgt);
