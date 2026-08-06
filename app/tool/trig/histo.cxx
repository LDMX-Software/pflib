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
using the_clock = std::chrono::high_resolution_clock;
using a_time_point = std::chrono::time_point<the_clock>;
static std::optional<a_time_point> time_of_last_clear{};

ENABLE_LOGGING();

std::optional<double> get_collection_time(a_time_point now) {
  using namespace std::literals;
  if (time_of_last_clear) {
    return (now - time_of_last_clear.value()) / 1.0s;
  } else {
    pflib_log(warn) << "There hasn't be a CLEAR recently,"
                       " so we don't know the collection time and"
                       " the histograms are probably saturated!";
    return {};
  }
}

void histo(const std::string& cmd, Target* tgt) {
  static FWHistoPool hist_pool{0};

  if (cmd == "CLEAR") {
    hist_pool.clear();
    time_of_last_clear = the_clock::now();
  }

  if (cmd == "DEBUG") {
    static int code = 0b01010101;
    code = pftool::readline_int("debug code:", code, true);
    hist_pool.debug(code);
  }

  if (cmd == "READ") {
    static int ihist = 0;
    ihist = pftool::readline_int("Which histogram?", ihist);
    auto now = the_clock::now();
    std::array<uint32_t, 256> hist = hist_pool.read(ihist);
    std::optional<double> collection_time = get_collection_time(now);
    pflib_log(info) << "accumulated histogram for "
                    << collection_time.value_or(0) << "s";
    bool raw_counts = true;
    if (collection_time) {
      raw_counts =
          pftool::readline_bool("Show raw counts (y) or rate (n)?", raw_counts);
    }
    for (std::size_t i{0}; i < hist.size(); i++) {
      printf("%3ld ", i);
      if (raw_counts) {
        printf("%u", hist[i]);
      } else {
        printf("%0.4e", hist[i] / collection_time.value());
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
      file << FWHistoPool::to_json(hist, ihist, collection_time.value_or(0));
    }
  }

  if (cmd == "DUMP") {
    bool decoded = pftool::readline_bool("Decoded sums instead of encoded sums?", true);
    std::array<std::array<uint32_t, 256>, 8> hists;
    auto now = the_clock::now();
    int offset{decoded?0:8};
    for (int ihist{0}; ihist < hists.size(); ihist++) {
      hists[ihist] = hist_pool.read(offset+ihist);
    }

    std::optional<double> collection_time = get_collection_time(now);
    pflib_log(info) << "accumulated histograms for "
                    << collection_time.value_or(0) << "s";

    if (pftool::readline_bool("Show histograms in terminal?", true)) {
      bool raw_counts = true;
      if (collection_time) {
        raw_counts = pftool::readline_bool("Show raw counts (y) or rate (n)?",
                                           raw_counts);
      }
      printf("bin : %10u %10u %10u %10u %10u %10u %10u %10u\n", 0, 1, 2, 3, 4,
             5, 6, 7);
      for (std::size_t i{0}; i < hists[0].size(); i++) {
        printf("%3ld :", i);
        for (int ihist{0}; ihist < hists.size(); ihist++) {
          if (raw_counts) {
            printf(" %10u", hists[ihist][i]);
          } else {
            printf(" %10.4e", hists[ihist][i] / collection_time.value());
          }
        }
        printf("\n");
      }
    }

    if (pftool::readline_bool("Store histograms in JSON file for plotting?",
                              false)) {
      std::string def_prefix{"fwhist-dump-"};
      def_prefix += decoded?"decoded":"encoded";
      auto path = pftool::readline_path(def_prefix, ".json");
      std::ofstream file{path};
      if (not file.is_open()) {
        PFEXCEPTION_RAISE("FileOpen", "Unable to open " + path);
      }
      file << FWHistoPool::to_json(hists, decoded, collection_time.value_or(0));
    }
  }
}
