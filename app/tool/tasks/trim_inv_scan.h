#pragma once

#include "../pftool.h"

/**
 * TASKS.TRIM_INV_SCAN
 * 
 * Scan TRIM_INV Parameter across all 63 parameter points for each channel
 * Check ADC Pedstals for each parameter point
 * Used to trim pedestals within each link
 */
void trim_inv_scan(Target* tgt);
