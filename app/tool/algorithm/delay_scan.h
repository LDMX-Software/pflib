#pragma once

#include "../pftool.h"
#include "pflib/Target.h"

/**
 * Using the non-linearity_scan algorithm, the script goes through different ADC-bit delay settings, mimizing the largest INL value. 
 * The "optimal" delay settings are chosen as ones that have the smallest maximum INL value. 
 * The "optimal bx" is also calculated as input for the non-linearity scan, to save time by identifying the bx that corresponds to the peak,
 * which is the only relevant ADC range.
 */

namespace pflib::algorithm {

	template <class EventPacket>
	std::map<std::string, std::map<std::string, uint64_t>> delay_scan(Target* tgt, ROC& roc);
}
