#pragma once
#include "../pftool.h"

/**
 * TASKS.TOT_VREF_SCAN
 *
 * Scan TOT_VREF for both halves of the chip
 * Check ADC Pedestals for each point
 */
void tot_vref_scan(Target* tgt);
