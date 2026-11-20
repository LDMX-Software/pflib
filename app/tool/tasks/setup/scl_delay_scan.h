#pragma once

#include "../../pftool.h"

/**
 * TASKS.SETUP.SCL_DELAY_SCAN
 *
 * The ECal motherboard has four selectable delay paths on SCL. Which one
 * gives clean I2C with the HGCROCs changes from power-up to power-up,
 * so count the ROC I2C errors on each path and keep the best one.
 * Run it before loading the ROC configuration.
 */
void scl_delay_scan(Target* tgt);
