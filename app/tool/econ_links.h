#pragma once

#include "./pftool.h"
#include "pflib/Target.h"
#include "pflib/packing/Hex.h"

using pflib::packing::hex;

// Detect number of ECON links active
// use of inline, so that only one copy of the function exists
// inline int determine_n_links(pflib::Target* tgt)
int determine_n_links(pflib::Target* tgt) {
  auto econ = tgt->econ(pftool::state.iecon);
  int n_links = 0;

  for (int ch = 0; ch < 12; ch++) {
    std::string name = std::to_string(ch) + "_ENABLE";
    auto val = econ.readParameter("ERX", name);
    std::cout << "Channel enabled " << ch << " = " << val << ", " << hex(val)
              << std::endl;

    if (val == 1) {
      n_links += 1;
    }
  }
  return n_links;
}