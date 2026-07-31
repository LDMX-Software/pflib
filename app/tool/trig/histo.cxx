/**
 * @file histo.cxx
 */
#include "histo.h"

#include "pflib/TRIG.h"
#include "pflib/zcu/zcu_trig.h"

void histo(const std::string& cmd, Target* tgt) {
  pflib::TRIG* trig = tgt->trig();
  if (!trig) return;
  auto* ztrig = dynamic_cast<pflib::zcu::ZCUtrig*>(trig);
  if (!ztrig) return;

  if (cmd == "CLEAR") {
    ztrig->clear_histograms();
  }

  if (cmd == "DEBUG") {
    static int code = 0;
    code = pftool::readline_int("debug code:", code, true);
    ztrig->debug_histogram(code);
  }

  if (cmd == "READ") {
    static int ihist = 0;
    ihist = pftool::readline_int("Which histogram?", ihist); 
    std::vector<uint32_t> hist = ztrig->read_histogram(ihist);
    for (std::size_t i{0}; i < hist.size(); i++) {
      printf("%3d %u\n", i, hist[i]);
    }
  }

  if (cmd == "DUMP") {
    printf("bin : %10u %10u %10u %10u %10u %10u %10u %10u\n", 0, 1, 2, 3, 4, 5, 6, 7);
    std::array<std::vector<uint32_t>, 8> hists;
    std::array<unsigned int, 8> total;
    total.fill(0);
    for (int ihist{0}; ihist < hists.size(); ihist++) {
      hists[ihist] = ztrig->read_histogram(ihist);
    }
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
}
