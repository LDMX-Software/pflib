/**
 * @file histo.cxx
 */
#include "histo.h"

#include <chrono>
#include <fstream>

#include "FWHistoPool.h"
#include "pflib/TRIG.h"
#include "pflib/utility/string_format.h"

using pflib::utility::string_format;

ENABLE_LOGGING();

FWHistoPool::FillValue prompt_fill_value() {
  static int choice = 0;
  choice = pftool::readline_int("Fill Value Options:\n  0: decoded sums\n  1: encoded sums\n  2: test hist\nWhich do you choose?", choice);
  if (choice < 0 or choice > 2) {
    PFEXCEPTION_RAISE("BadChoice", "Chosen fill value is not one of the options");
  }
  return FWHistoPool::FillValue(choice);
}

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
    auto hist = hist_pool.read(prompt_fill_value(), ihist);
    pflib_log(info) << "accumulated histogram for "
                    << hist.collection_time() << "s";
    bool raw_counts = true;
    if (hist.collection_time() > 0) {
      raw_counts =
          pftool::readline_bool("Show raw counts (y) or rate (n)?", raw_counts);
    }
    for (std::size_t i{0}; i < hist.values().size(); i++) {
      printf("%3ld ", i);
      if (raw_counts) {
        printf("%u", hist.values()[i]);
      } else {
        printf("%0.4e", hist.values()[i] / hist.collection_time());
      }
      printf("\n");
    }
    if (pftool::readline_bool("Store histogram in JSON file for plotting?",
                              false)) {
      auto path =
          pftool::readline_path(string_format("fwhist-%d", ihist), ".json");
      std::ofstream file(path);
      if (not file.is_open()) {
        PFEXCEPTION_RAISE("FileOpen", "Unable to open " + path);
      }
      file << hist.to_json();
    }
  }

  if (cmd == "DUMP") {
    auto hist = hist_pool.read(prompt_fill_value());
    pflib_log(info) << "accumulated histograms for "
                    << hist.collection_time() << "s";

    if (pftool::readline_bool("Show histograms in terminal?", true)) {
      bool raw_counts = true;
      if (hist.collection_time() > 0) {
        raw_counts = pftool::readline_bool("Show raw counts (y) or rate (n)?",
                                           raw_counts);
      }
      printf("bin : %10u %10u %10u %10u %10u %10u %10u %10u\n", 0, 1, 2, 3, 4,
             5, 6, 7);
      for (std::size_t i{0}; i < hist.values()[0].size(); i++) {
        printf("%3ld :", i);
        for (int ihist{0}; ihist < hist.values().size(); ihist++) {
          if (raw_counts) {
            printf(" %10u", hist.values()[ihist][i]);
          } else {
            printf(" %10.4e", hist.values()[ihist][i] / hist.collection_time());
          }
        }
        printf("\n");
      }
    }

    if (pftool::readline_bool("Store histograms in JSON file for plotting?",
                              false)) {
      std::string def_prefix{"fwhist-dump-"};
      switch (hist.fill_type()) {
        case FWHistoPool::FillValue::DecodedSum:
          def_prefix += "decoded";
          break;
        case FWHistoPool::FillValue::EncodedSum:
          def_prefix += "encoded";
          break;
        default:
          break;
      }
      auto path = pftool::readline_path(def_prefix, ".json");
      std::ofstream file{path};
      if (not file.is_open()) {
        PFEXCEPTION_RAISE("FileOpen", "Unable to open " + path);
      }
      file << hist.to_json();
    }
  }
}
