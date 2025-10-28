#pragma once

#include "../pftool.h"

/**
 * TASKS.NOINV_VREF_SCAN
 *
 * Perform NOINV_VREF scan for each link
 * used to trim adc pedestals between two links
 * same as inv_vref_scan with noinv_vref parameter instead
 */
void noinv_vref_scan(Target* tgt);
