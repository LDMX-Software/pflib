#pragma once

#include "../pftool.h"

void align_phase_word(Target* tgt);

void print_roc_status(pflib::ROC& roc);

uint32_t build_channel_mask(std::vector<int>& channels);
