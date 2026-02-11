#pragma once
#include "../pftool.h"

/**
 * TASKS.LEVEL_PEDESTALS_INV_VREF
 *
 * 1) Run pedestal leveling and apply automatically
 * 2) Run INV_VREF scan (defaults: nevents=100, ch0=17, ch1=51)
 * 3) Compute optimal INV_VREF and apply automatically
 * 4) Save combined YAML (INV_VREF + pedestal params)
 */
void level_pedestals_inv_vref(Target* tgt);
