#pragma once

#include "../pftool.h"  // I don't think this is actually necessary for now but it won't hurt
#include "pflib/Target.h"

/**
 * @namespace pflib::algorithm
 * housing of higher-level methods for repeatable tasks
 */
namespace pflib::algorithm {

template <class EventPacket>
void delay_scan(Target* tgt);

}  // namespace pflib::algorithm
