/**
 * @file histo.cxx
 */
#include "histo.h"

#include <fstream>

#include "FWHistoPool.h"
#include "pflib/TRIG.h"
#include "pflib/utility/string_format.h"

using pflib::utility::string_format;
void histo(const std::string& cmd, Target* tgt) {
  static FWHistoPool hist_pool{0};

  if (cmd == "CLEAR") {
    hist_pool.clear();
  }

  if (cmd == "DEBUG") {
    static int code = 0b01010101;
    code = pftool::readline_int("debug code:", code, true);
    hist_pool.debug(code);
  }

  if (cmd == "READ") {
    static int ihist = 0;
    ihist = pftool::readline_int("Which histogram?", ihist);
    std::array<uint32_t, 256> hist = hist_pool.read(ihist);
    for (std::size_t i{0}; i < hist.size(); i++) {
      printf("%3d %u\n", i, hist[i]);
    }
    if (pftool::readline_bool("Store histogram in JSON file for plotting?",
                              false)) {
      auto path =
          pftool::readline_path(string_format("fwhist-%d", ihist), ".json");
      std::ofstream file(path);
      if (not file.is_open()) {
        PFEXCEPTION_RAISE("FileOpen", "Unable to open " + path);
      }
      file << FWHistoPool::to_json(hist, ihist);
    }
  }

  if (cmd == "DUMP") {
    // since the histograms fill so fast,
    // we read BEFORE asking what to write to enable a RESET->DUMP to
    // be able to happen quickly
    std::array<std::array<uint32_t, 256>, 8> hists;
    std::array<unsigned int, 8> total;
    total.fill(0);
    for (int ihist{0}; ihist < hists.size(); ihist++) {
      hists[ihist] = hist_pool.read(ihist);
    }

    if (pftool::readline_bool("Show histograms in terminal?", true)) {
      printf("bin : %10u %10u %10u %10u %10u %10u %10u %10u\n", 0, 1, 2, 3, 4,
             5, 6, 7);
      for (std::size_t i{0}; i < hists[0].size(); i++) {
        printf("%3d :", i);
        for (int ihist{0}; ihist < hists.size(); ihist++) {
          printf(" %10u", hists[ihist][i]);
          total[ihist] += hists[ihist][i];
        }
        printf("\n");
      }
      printf("tot :");
      for (int ihist{0}; ihist < total.size(); ihist++) {
        printf(" %10u", total[ihist]);
      }
      printf("\n");
    }

    if (pftool::readline_bool("Store histograms in JSON file for plotting?",
                              false)) {
      auto path = pftool::readline_path("fwhist-dump", ".json");
      std::ofstream file{path};
      if (not file.is_open()) {
        PFEXCEPTION_RAISE("FileOpen", "Unable to open " + path);
      }
      file << FWHistoPool::to_json(hists);
    }
  }
}
