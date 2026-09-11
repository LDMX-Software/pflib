#pragma once

#include "../pftool.h"

/**
 * TASKS.LED_BIAS_SCAN
 *
 * Used to scan different SiPM or LED DAC values/biases.
 * Both can either be varied over a range, or kept constant if the same value is
 * entered for the start and stop value. One can choose how many ports have CMBs
 * connected An LED flash on one CMB flashes into four HGCROC channels
 * simultaneously, thus the pulses of all four respective channels are recorded
 * in the csv file.
 *
 */
void led_bias_scan(Target* tgt);
