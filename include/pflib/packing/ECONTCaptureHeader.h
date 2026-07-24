#pragma once
#ifndef PFLIB_PACKING_ECONTCAPTUREHEADER_H
#define PFLIB_PACKING_ECONTCAPTUREHEADER_H

#include <cstdint>
#include <span>

#include "pflib/logging/Logging.h"

namespace pflib::packing {

/**
 * The econt_buffer_manager firmware block inserts a pair of headers
 * with the same format for the data capture along the trigger path
 * and the capture of the output of the trigger algorithm.
 */
class ECONTCaptureHeader {
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get("decoding")};

  /// version of capture format
  int version_;
  /// id for econ-t we captured
  int econ_id_;
  /// number of pre-samples within this capture
  int pre_samples_;
  /// total number of samples within the capture
  int n_samples_;
  /// length of entire capture in 32b words including this header
  int length_;
  /// number of links included in this capture
  int n_links_;

 public:
  void from(std::span<uint32_t> data);
  int version() const;
  int econ_id() const;
  int length() const;
  int n_links() const;
  int pre_samples() const;
  int n_samples() const;
};

}  // namespace pflib::packing

#endif
