#pragma once

#include "../pftool.h"

/*
 * TASKS.CHECK_PEDESTAL
 *
 * Take a pedestal run and compare the median of every channel with the
 * expected_pedestal of a calibration set (default: the target calib file).
 * Large offsets mean the loaded calibration went stale or the hardware moved.
 */
void check_pedestal(Target* tgt);
