#pragma once

#include "pflib/Target.h"

struct hgcroc_connection {
  hgcroc_connection();
  ~hgcroc_connection() = default;
  static std::unique_ptr<pflib::Target> tgt;
};
