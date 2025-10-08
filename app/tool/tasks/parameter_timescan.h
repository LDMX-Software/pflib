#pragma once

#include "../pftool.h"

/*
 * TASKS.PARAMETER_TIMESCAN
 *
 * Scan a internal calibration pulse in time by varying the charge_to_l1a
 * and top.phase_strobe parameters, and then change given parameters pulse-wise.
 * In essence, an implementation of gen_scan into charge_timescan, to see how
 * pulse shapes transform with varying parameters.
 *
 */
void parameter_timescan(Target* tgt);
