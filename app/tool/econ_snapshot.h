#pragma once

#include "pftool.h"

#include "pflib/ECON.h"

/// construct mask for enabling specific channels
uint32_t build_channel_mask(std::vector<int>& channels);

/// do a snapshot for the input channels
void econ_snapshot(pflib::Target* tgt, pflib::ECON& econ, std::vector<int>& channels);

