#pragma once
#include "../pftool.h"
#include "decode_multi_sample.h"

/**
 * share various settings related to timein
 * without permanently changing the chip
 */
struct TimeInSettings {
  int charge_to_l1a;
  int l1offset;
  int pipeline;
  static TimeInSettings last;
};

void timein(Target* tgt);
