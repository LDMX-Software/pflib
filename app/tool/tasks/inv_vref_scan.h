#pragma once

#include "../pftool.h"

/**
 * TASKS.INV_VREF_SCAN
 * 
 * Perform INV_VREF scan for each link 
 * used to trim adc pedestals between two links
 */
void inv_vref_scan(Target* tgt);
