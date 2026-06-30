
#pragma once

#include "../daq_run.h"
#include "../pftool.h"

/**
 * TASKS.SET_PHASE
 *
 * Scan over charge_to_l1a, find max when phase_strobe is 0.
 * Scan over phase_strobe, and find value where charge_to_l1a + 1 is maximum.
 */
void scan_phase_strobe(Target* tgt, pflib::ROC& roc, int i_roc, DecodeAndBuffer& buffer, int nevents);
void scan_phase_ck(Target* tgt, DecodeAndBuffer& buffer, int nevents);
int peak_bx(Target* tgt, pflib::ROC& roc, int i_roc, DecodeAndBuffer& buffer, int nevents);
void examine_phase(Target* tgt);
