#pragma once

#include "../pftool.h"

/**
 * TASKS.CHANNEL_WISE_CALIB_SCAN
 *
 * Scans each channel sequentially, for varying calib values.
 * Used for testing if TOA and TOT are triggering mainly.
 *
 */
void channel_wise_calib_scan(Target* tgt);
