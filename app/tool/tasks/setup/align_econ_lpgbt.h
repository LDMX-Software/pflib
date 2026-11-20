#pragma once

#include "../../pftool.h"
#include "pflib/zcu/lpGBT_ICEC_ZCU_Simple.h"


/**
 * make sure the ECON and its associated lpGBT agree on what a bit is
 */
void align_econ_lpgbt_bit(Target* tgt, pflib::ECON& econ, int iecon,
                          bool check_all_phases, bool is_hcal);

/**
 * make sure the ECON and its associated lpGBT agree on what a word is
 */
void align_econ_lpgbt_word(Target* tgt, pflib::ECON& econ, int iecon,
                           bool check_all_phases, uint32_t idle_pattern,
                           bool is_hcal);
