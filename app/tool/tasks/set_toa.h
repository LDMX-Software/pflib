#pragma once

#include "../pftool.h"

/*
 * TASKS.SET_TOA
 *
 * Do a charge injection run for a given channel, find the toa efficiency, and set
 * the toa threshold to the first point where the toa efficiency is 1. 
 * This corresponds to a point above the pedestal where the pedestal shouldn't 
 * fluctuate and trigger the toa.
 *
 * Things that might be improved upon:
 * the number of events nevents, which determines our toa efficiency.
 * The calib value which defines a small pulse. 
 *
 */
void set_toa(Target* tgt, pflib::ROC& roc, int channel);
