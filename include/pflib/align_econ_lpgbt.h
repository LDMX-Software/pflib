#pragma once
#ifndef PFLIB_ALIGNECONLPGBT_H
#define PFLIB_ALIGNECONLPGBT_H

#include "pflib/zcu/lpGBT_ICEC_ZCU_Simple.h"

namespace pflib {

class Target;

/**
 * make sure the ECON and its associated lpGBT agree on what a bit is
 */
void align_econ_lpgbt_bit(Target* tgt, int iecon, bool check_all_phases,
                          bool is_hcal);

/**
 * make sure the ECON and its associated lpGBT agree on what a word is
 */
void align_econ_lpgbt_word(Target* tgt, int iecon, bool check_all_phases,
                           uint32_t idle_pattern, bool is_hcal);

}  // namespace pflib

#endif
