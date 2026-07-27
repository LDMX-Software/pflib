#pragma once
#ifndef PFLIB_HCALTARGET_H
#define PFLIB_HCALTARGET_H

#include "pflib/Target.h"

namespace pflib {

class Bias;

/**
 * An HcalTarget is a Target with access to the biasing chips
 */
class HcalTarget : public Target {
 public:
  virtual Bias& bias(int which) = 0;
};

}  // namespace pflib

#endif
