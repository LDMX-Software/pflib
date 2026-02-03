#pragma once

#include "../pftool.h"  // I don't think this is actually necessary for now but it won't hurt
#include "pflib/Target.h"

/**
 * @namespace pflib::algorithm
 * housing of higher-level methods for repeatable tasks
 */
namespace pflib::algorithm {

template <class EventPacket>
double inl_scan(Target* tgt, ROC& roc, size_t& n_events, auto& channel_page,
                auto& refvol_page, auto& globalanalog_page, auto& buffer,
                std::array<int, 3> delays);

}  // namespace pflib::algorithm
