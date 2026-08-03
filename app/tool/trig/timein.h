#pragma once
#include "../pftool.h"
#include "decode_multi_sample.h"

/**
 * share various settings related to timein
 * without permanently changing the chip
 */
struct TimeInSettings {
  int charge_to_l1a{0};
  int l1offset{0};
  int pipeline{0};
  static TimeInSettings last;
  void init(Target* tgt);
  void apply(Target* tgt) const;
  void update(int new_l1offset, int new_pipeline, int new_charge_to_l1a = -1);
};

void timein(Target* tgt);
