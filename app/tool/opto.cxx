/**
 * @file opto.cxx
 * OPTO menu commands
 */
#include "pflib/OptoLink.h"
#include "pftool.h"

ENABLE_LOGGING();

void opto_render(Target* tgt) {
  if (tgt->opto_link_names().empty()) {
    pflib_log(error) << "no optical links connected for this target";
  }
}

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
  static std::string olink_name;

  if (cmd == "CHOOSE" or olink_name.empty()) {
    auto names = target->opto_link_names();
    for (auto name : names) {
      std::cout << "  " << name << "\n";
    }
    olink_name = pftool::readline("What optical link should we connect to? ",
                                  names, olink_name);
  }

  auto& olink{target->get_opto_link(olink_name)};

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
  if (cmd == "SOFTRESET") {
    olink.soft_reset_link();
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
    pftool::menu("OPTO", "Optical Link Functions", opto_render, NEED_FIBER)
        ->line("CHOOSE", "Choose optical link to connect to", opto)
        ->line("FULLSTATUS", "Get full status", opto)
        ->line("SOFTRESET", "soft reset optical link", opto)
        ->line("RESET", "hard reset optical link (only do while frontend is OFF)", opto)
        ->line("POLARITY", "Adjust the polarity", opto)
        ->line("LINKTRICK", "Cycle into/out of fixed speed to get SFP to lock",
               opto);
}
