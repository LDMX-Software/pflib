#pragma once

#include "../pftool.h"

/*
 * TASKS.SET_ZS_THRESHOLDS
 *
 * Set the ECON-D zero suppression threshold of every channel to its
 * pedestal plus a margin, c = round(pedestal) + T. The pedestal comes from
 * a pass-through pedestal run (RUN) or from expected_pedestal of a
 * calibration set (CALIB). The calib slot (18) stays suppressed. The
 * settings are written to a YAML file that the YAML source applies again.
 *
 * The ZS pages are only in the econd::with_zs register map, so ECON.POKE
 * and ECON.LOAD cannot reach them.
 */
void set_zs_thresholds(Target* tgt);
