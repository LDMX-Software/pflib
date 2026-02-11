#pragma once

#include "../pftool.h"

/**
 * TASKS.VREF_2D_SCAN
 *
 * Scan a internal calibration pulse in time by varying the charge_to_l1a
 * and top.phase_strobe parameters
 */
void vref_2d_scan(Target* tgt);
