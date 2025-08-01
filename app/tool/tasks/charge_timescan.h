#pragma once

#include "../pftool.h"

/**
 * TASKS.CHARGE_TIMESCAN
 *
 * Scan a internal calibration pulse in time by varying the charge_to_l1a
 * and top.phase_strobe parameters
 */
void charge_timescan(Target* tgt);
