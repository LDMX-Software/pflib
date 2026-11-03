#pragma once

#include "../pftool.h"

/*
 * TASKS.SET_ZS_THRESHOLDS
 *
 * Set the ECON-D zero suppression threshold of every channel to its
 * pedestal plus a margin, c = round(pedestal) + T. The pedestal comes from
 * a pass-through pedestal run or from expected_pedestal of a calibration
 * set. The calib slot (18) stays suppressed. The settings are also written
 * to a YAML file that ECON.LOAD can apply again later.
 */
void set_zs_thresholds(Target* tgt);
