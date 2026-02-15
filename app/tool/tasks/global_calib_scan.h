#pragma once

#include "../pftool.h"

/**
 * TASKS.GLOBAL_CALIB_SCAN
 *
 * Scan a internal calibration pulse in time by varying the charge_to_l1a
 * and top.phase_strobe parameters
 */
void global_calib_scan(Target* tgt);
