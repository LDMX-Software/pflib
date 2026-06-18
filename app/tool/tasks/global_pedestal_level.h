#pragma once
#include "../pftool.h"

/**
 * TASKS.GLOBAL_PEDESTAL_LEVEL
 *
 * Scan INV_VREF for both halves of the chip
 * Set NOINV_VREF to 612
 *
 */
void global_pedestal_level(Target* tgt);
