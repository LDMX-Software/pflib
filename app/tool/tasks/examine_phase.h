
#pragma once

#include "../daq_run.h"
#include "../pftool.h"

/**
 * TASKS.EXAMINE_PHASE
 *
 * Scans over charge_to_l1a and finds bx location of charge pulse.
 * Scans over phase_ck and prints values. Optimal phase_ck is when pedestal adc is at a maximum.
 * Scans over phase_strobe and prints values.
 * charge_to_l1a should be placed at the phase_strobe and bx where adc is at a maximum.
 */
void scan_phase_strobe(Target* tgt, pflib::ROC& roc, int i_roc, DecodeAndBuffer& buffer, int nevents);
void scan_phase_ck(Target* tgt, DecodeAndBuffer& buffer, int nevents);
int peak_bx(Target* tgt, pflib::ROC& roc, int i_roc, DecodeAndBuffer& buffer, int nevents);
void examine_phase(Target* tgt);
