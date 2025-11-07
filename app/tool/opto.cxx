/**
 * @file opto.cxx
 * OPTO menu commands
 */
#include "pflib/zcu/lpGBT_ICEC_ZCU_Simple.h"
#include "pflib/zcu/zcu_optolink.h"
#include "pftool.h"

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
  static const int iolink = 0;
  const std::vector<pflib::OptoLink*>& olinks=target->optoLinks();

  if (olinks.size()<=iolink) {
    printf("Requested optical link does not exist\n");
    return;
  }
  pflib::OptoLink& olink=*olinks[iolink];

  if (cmd == "FULLSTATUS") {
    printf("Polarity -- TX: %d  RX: %d\n", olink.get_tx_polarity(),
           olink.get_rx_polarity());
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
  if (cmd == "RESET") {
    olink.reset_link();
  }
  if (cmd == "POLARITY") {
    bool change;
    printf("Polarity -- TX: %d  RX: %d\n", olink.get_tx_polarity(),
           olink.get_rx_polarity());
    change = pftool::readline_bool("Change TX polarity? ", false);
    if (change) olink.set_tx_polarity(!olink.get_tx_polarity());
    change = pftool::readline_bool("Change RX polarity? ", false);
    if (change) olink.set_rx_polarity(!olink.get_tx_polarity());
  }
  if (cmd == "LINKTRICK") {
    olink.run_linktrick();
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
