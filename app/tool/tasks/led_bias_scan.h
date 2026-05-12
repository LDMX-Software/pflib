#pragma once

#include "../pftool.h"

/**
 * TASKS.LED_BIAS_SCAN
 *
 * Used to scan different SiPM or LED DAC values/biases. 
 * Both can either be varied over a range, or kept constant if the same value is entered for the start and stop value.
 * Each LED flashes into all four channels on the respective CMB, but nevertheless a channel range can be chosen.
 *
 */
void led_bias_scan(Target* tgt);