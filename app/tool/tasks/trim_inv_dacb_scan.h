#pragma once

#include "../pftool.h"

/**
 * TASKS.TRIM_INV_DACB_SCAN
 *
 * Scan TRIM_INV and DACB
 * TRIM_INV scan done with DACB = 0 and DACB scan done with TRIM_INV = 0
 * Check ADC Pedstals for each parameter point
 * Used to trim pedestals within each link
 */
void trim_inv_dacb_scan(Target* tgt);
