#pragma once

#include "../pftool.h"

/**
 * TASKS.VREF_2D_SCAN
 *
 * Scans pedestal values for changing inv_vref and noinv_vref parameters.
 * Arguments:
 *  - nevents sets the number of samples per timepoint.
 *  - stepsize sets the steps between each inv_vref and noinv_vref parameter,
 * respectively.
 *  - fname sets the name of the output csv file.
 */
void vref_2d_scan(Target* tgt);
