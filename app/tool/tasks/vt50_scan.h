#pragma once

#include "../pftool.h"

/**
 * TASKS.VT50_SCAN
 *
 * Scans the calib range for the v_t50 of the tot.
 * The v_t50 is the point where the tot triggers 50 percent of the time.
 * We check the tot efficiency at every calib value and use a recursive method
 * to hone in on the v_t50.
 *
 * The tot efficiency is calculated depending on the number of events given. The
 * higher the number of events, the more accurate tot efficiency.
 *
 * To hone in on the v_t50, I constructed two methods: binary and bisectional
 * search. Both methods work, altough the binary search sometimes scans the same
 * parameter point twice, giving rise to tot efficiencies larger than 1. This
 * can be adjusted for in analysis.
 *
 */
void vt50_scan(Target* tgt);
