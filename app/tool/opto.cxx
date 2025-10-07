/**
 * @file opto.cxx
 * OPTO menu commands
 */
#include "pftool.h"

#include "pflib/zcu/zcu_optolink.h"
#include "pflib/zcu/lpGBT_ICEC_ZCU_Simple.h"

/**
 * Interaction with Optical links
 *
 * ## Commands
 * - FULLSTATUS : printout status of the optical links
 * - RESET : call reset_link on the optical links
 * - POLARITY : change the polarity of the optical links
 * - LINKTRICK : try a simple trick to re-align the optical links
 */
void opto(const std::string& cmd, Target* target) {
  static const int irx = 8;
  static const int itx = 8;
  static const std::string target_name = "standardLpGBTpair-0";
  static pflib::zcu::OptoLink olink(target_name);
  if (cmd == "FULLSTATUS") {
    printf("Polarity -- TX: %d  RX: %d\n", olink.get_polarity(itx, false),
           olink.get_polarity(irx, true));
    std::map<std::string, uint32_t> info;
    info = olink.opto_status();
    printf("Optical status:\n");
    for (auto i : info) {
      printf("  %-20s : 0x%04x\n", i.first.c_str(), i.second);
    }
    info = olink.opto_rates();
    printf("Optical rates:\n");
    for (auto i : info) {
      printf("  %-20s : %.3f MHz (0x%04x)\n", i.first.c_str(), i.second / 1e3,
             i.second);
    }
  }
  if (cmd == "RESET") {    olink.reset_link();
  }
  if (cmd == "POLARITY") {
    bool change;
    printf("Polarity -- TX: %d  RX: %d\n", olink.get_polarity(itx, false),
           olink.get_polarity(irx, true));
    change = pftool::readline_bool("Change TX polarity? ", false);
    if (change) olink.set_polarity(!olink.get_polarity(itx, false), itx, false);
    change = pftool::readline_bool("Change RX polarity? ", false);
    if (change) olink.set_polarity(!olink.get_polarity(irx, true), irx, true);
  }
  if (cmd == "LINKTRICK") {
    // configure lpGBT for Internal Communication (IC)
    int chipaddr = 0x78 | 0x04;
    pflib::zcu::lpGBT_ICEC_Simple ic(target_name, false, chipaddr);
    // pass configuration to lpGBT object
    pflib::lpGBT lpgbt(ic);
    lpgbt.write(0x128, 0x5);
    sleep(1);
    lpgbt.write(0x128, 0x0);
  }
}

namespace {
auto optom =
    pftool::menu("OPTO", "Optical Link Functions")
        ->line("FULLSTATUS", "Get full status", opto)
        ->line("RESET", "Reset optical link", opto)
        ->line("POLARITY", "Adjust the polarity", opto)
        ->line("LINKTRICK", "Cycle into/out of fixed speed to get SFP to lock",
               opto);
}
