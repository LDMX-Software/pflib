#pragma once

#include "../pftool.h"

/**
 * TASKS.SAMPLING_PHASE_SCAN
 *
 * Scan over phase_ck, do pedestals.
 * Used to align clock phase. Inspired by the document "Handling
 * of different HGCAL commissiong run types: procedures, results,
 * expected" by Amendola et al.
 */
void sampling_phase_scan(Target* tgt);
