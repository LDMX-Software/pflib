/**
 * @file expert.cxx
 * EXPERT menu commands
 */
#include "pftool.h"

/**
 * Direct I2C commands
 *
 * We construct a raw I2C interface by passing the wishbone from the target
 * into the pflib::I2C class.
 *
 * ## Commands
 * - BUS : pflib::I2C::set_active_bus, default retrieved by
 * pflib::I2C::get_active_bus
 * - WRITE : pflib::I2C::write_byte after passing 1000 to
 * pflib::I2C::set_bus_speed
 * - READ : pflib::I2C::read_byte after passing 100 to pflib::I2C::set_bus_speed
 * - MULTIREAD : pflib::I2C::general_write_read
 * - MULTIWRITE : pflib::I2C::general_write_read
 *
 * @param[in] cmd I2C command
 * @param[in] pft active target
 */
static void i2c(const std::string& cmd, Target* target) {
  static uint32_t addr = 0;
  static int waddrlen = 1;
  static int i2caddr = 0;
  static std::string chosen_bus;

  printf(" Current bus: %s\n", chosen_bus.c_str());

  if (cmd == "BUS") {
    std::vector<std::string> busses = target->i2c_bus_names();
    printf("\n\nKnown I2C busses:\n");
    for (auto name : target->i2c_bus_names()) printf(" %s\n", name.c_str());
    chosen_bus = pftool::readline("Bus to make active: ", chosen_bus);
    if (chosen_bus.size() > 1) target->get_i2c_bus(chosen_bus);
  }
  if (chosen_bus.size() < 2) return;
  pflib::I2C& i2c = target->get_i2c_bus(chosen_bus);
  if (cmd == "WRITE") {
    i2caddr = pftool::readline_int("I2C Target ", i2caddr);
    uint32_t val = pftool::readline_int("Value ", 0);
    i2c.set_bus_speed(1000);
    i2c.write_byte(i2caddr, val);
  }
  if (cmd == "READ") {
    i2caddr = pftool::readline_int("I2C Target", i2caddr);
    i2c.set_bus_speed(100);
    uint8_t val = i2c.read_byte(i2caddr);
    printf("%02x : %02x\n", i2caddr, val);
  }
  if (cmd == "MULTIREAD") {
    i2caddr = pftool::readline_int("I2C Target", i2caddr);
    waddrlen = pftool::readline_int("Read address length", waddrlen);
    std::vector<uint8_t> waddr;
    if (waddrlen > 0) {
      addr = pftool::readline_int("Read address", addr);
      for (int i = 0; i < waddrlen; i++) {
        waddr.push_back(uint8_t(addr & 0xFF));
        addr = (addr >> 8);
      }
    }
    int len = pftool::readline_int("Read length", 1);
    std::vector<uint8_t> data = i2c.general_write_read(i2caddr, waddr);
    for (size_t i = 0; i < data.size(); i++)
      printf("%02x : %02x\n", int(i), data[i]);
  }
  if (cmd == "MULTIWRITE") {
    i2caddr = pftool::readline_int("I2C Target", i2caddr);
    waddrlen = pftool::readline_int("Write address length", waddrlen);
    std::vector<uint8_t> wdata;
    if (waddrlen > 0) {
      addr = pftool::readline_int("Write address", addr);
      for (int i = 0; i < waddrlen; i++) {
        wdata.push_back(uint8_t(addr & 0xFF));
        addr = (addr >> 8);
      }
    }
    int len = pftool::readline_int("Write data length", 1);
    for (int j = 0; j < len; j++) {
      char prompt[64];
      sprintf(prompt, "Byte %d: ", j);
      int id = pftool::readline_int(prompt, 0);
      wdata.push_back(uint8_t(id));
    }
    i2c.general_write_read(i2caddr, wdata);
  }
}

/**
 * ELINKS menu commands
 *
 * We retrieve a reference to the current elinks object via
 * pflib::Target::elinks and then procede to the commands.
 *
 * ## Commands
 * - SPY : pflib::Elinks::spy
 * - AUTO :
 * - BITSLIP : pflib::Elinks::setBitslip
 * - SPY : Target::Elinks::spy
 * - PHASE : pflib::Elinks::setAlignPhase
 * - HARD_RESET : pflib::Elinks::resetHard
 * - SCAN : pflib::Elinks::scanAlign
 * - STATUS : pflib::Target::elinkStatus with std::cout input
 *
 * @param[in] cmd ELINKS command
 * @param[in] pft active target
 */
static void elinks(const std::string& cmd, Target* pft) {
  pflib::Elinks& elinks = pft->elinks();
  if (cmd == "SPY") {
    pftool::state.ilink =
        pftool::readline_int("Which elink? ", pftool::state.ilink);
    std::vector<uint32_t> spy = elinks.spy(pftool::state.ilink);
    for (size_t i = 0; i < spy.size(); i++)
      printf("%02d %08x\n", int(i), spy[i]);
  }
  if (cmd == "BITSLIP") {
    pftool::state.ilink =
        pftool::readline_int("Which elink? ", pftool::state.ilink);
    int bitslip = pftool::readline_int("Bitslip value : ",
                                       elinks.getBitslip(pftool::state.ilink));
    for (int jlink = 0; jlink < 8; jlink++) {
      if (pftool::state.ilink >= 0 && jlink != pftool::state.ilink) continue;
      elinks.setBitslip(jlink, bitslip);
    }
  }
  if (cmd == "DELAY") {
    pftool::state.ilink =
        pftool::readline_int("Which elink? ", pftool::state.ilink);
    int idelay = elinks.getAlignPhase(pftool::state.ilink);
    idelay = pftool::readline_int("Phase value: ", idelay);
    elinks.setAlignPhase(pftool::state.ilink, idelay);
  }

  if (cmd == "HARD_RESET") {
    elinks.resetHard();
  }

  if (cmd == "SCAN") {
    pftool::state.ilink =
        pftool::readline_int("Which elink?", pftool::state.ilink);

    int bp = elinks.scanAlign(pftool::state.ilink, true);
    printf("\n Best Point: %d\n", bp);
  }
  if (cmd == "AUTO") {
    std::cout
        << "In order to align the ELinks, we need to hard reset the HGCROC\n"
           "to force the trigger links to return idles.\n"
           "This will reset all parameters on the HGCROC to their defaults.\n"
           "You can use ROC.DUMP to write out the current HGCROC settings to\n"
           "a YAML file for later loading.\n";

    if (not pftool::readline_bool("Continue? ", false)) {
      return;
    }

    // store run mode _before_ hard reset
    // (hard reset sets run mode to off)
    std::vector<bool> running(pft->nrocs());
    for (int iroc = 0; iroc < pft->nrocs(); iroc++) {
      running[iroc] = pft->roc(iroc).isRunMode();
    }

    pft->hardResetROCs();

    for (int alink = 0; alink < elinks.nlinks(); alink++) {
      int apt = elinks.scanAlign(alink, false);
      elinks.setAlignPhase(alink, apt);
      int bpt = elinks.scanBitslip(alink);
      if (bpt >= 0) elinks.setBitslip(alink, bpt);
      std::vector<uint32_t> spy = elinks.spy(alink);
      printf(" %d Best phase : %d  Bitslip : %d  Spy: 0x%08x\n", alink, apt,
             bpt, spy[0]);
    }

    // reset runmode to what it was before alignment attempt
    for (int iroc = 0; iroc < pft->nrocs(); iroc++) {
      pft->roc(iroc).setRunMode(running[iroc]);
    }
  }
}

/**
 * Fast Control (FC) menu commands
 *
 * ## Commands
 * - SW_L1A : pflib::Backend::fc_sendL1A
 * - LINK_RESET : pflib::Backend::fc_linkreset
 * - BUFFER_CLEAR : pflib::Backend::fc_bufferclear
 * - COUNTER_RESET : pflib::FastControl::resetCounters
 * - FC_RESET : pflib::FastControl::resetTransmitter
 * - CALIB : pflib::Backend::fc_setup_calib
 * - MULTISAMPLE : pflib::FastControl::setupMultisample
 *   and pflib::rouge::RogueWishboneInterface::daq_dma_setup if that is how the
 * user is connected
 * - STATUS : print mutlisample status and Error and Command counters
 *   pflib::FastControl::getErrorCounters, pflib::FastControl::getCmdCounters,
 *   pflib::FastControl::getMultisampleSetup
 *
 * @param[in] cmd FC menu command
 * @param[in] pft active target
 */
static void fc(const std::string& cmd, Target* pft) {
  bool do_status = false;

  if (cmd == "SW_L1A") {
    pft->fc().sendL1A();
    printf("Sent SW L1A\n");
  }
  if (cmd == "LINK_RESET") {
    pft->fc().linkreset_rocs();
    printf("Sent LINK RESET\n");
  }
  if (cmd == "RUN_CLEAR") {
    pft->fc().clear_run();
    std::cout << "Cleared run counters" << std::endl;
  }
  if (cmd == "BUFFER_CLEAR") {
    pft->fc().bufferclear();
    printf("Sent BUFFER CLEAR\n");
  }
  if (cmd == "COUNTER_RESET") {
    pft->fc().resetCounters();
    do_status = true;
  }

  if (cmd == "CALIB") {
    int offset = pft->fc().fc_get_setup_calib();
    offset = pftool::readline_int("Calibration L1A offset?", offset);
    pft->fc().fc_setup_calib(offset);
  }
  
  if (cmd == "LED") {
    int offset = pft->fc().fc_get_setup_led();
    offset = pftool::readline_int("Calibration L1A offset for LED?", offset);
    pft->fc().fc_setup_led(offset);
  }

  if (cmd == "STATUS" || do_status) {
    static const std::map<int, std::string> bit_comments = {
        {0, "encoding errors"},
        {3, "l1a/read requests"},
        {4, "l1a NZS requests"},
        {5, "orbit/bcr requests"},
        {6, "orbit count resets"},
        {7, "internal calib pulse requests"},
        {8, "external calib pulse requests"},
        {9, "chipsync resets"},
        {10, "event count resets"},
        {11, "event buffer resets"},
        {12, "link reset roc-t"},
        {13, "link reset roc-d"},
        {14, "link reset econ-t"},
        {15, "link reset econ-d"},
    };
    /*
    bool multi;
    int nextra;
    pft->fc().getMultisampleSetup(multi,nextra);
    if (multi) printf(" Multisample readout enabled : %d extra L1a (%d total
    samples)\n",nextra,nextra+1); else printf(" Multisaple readout disabled\n");
    printf(" Snapshot: %03x\n",pft->wb->wb_read(1,3));
    uint32_t sbe,dbe;
    pft->fc().getErrorCounters(sbe,dbe);
    printf("  Single bit errors: %d     Double bit errors: %d\n",sbe,dbe);
    */
    std::map<std::string, uint32_t> cnt = pft->fc().getCmdCounters();
    for (const auto& pair : cnt) {
      printf("  %-30s: %10u \n", pair.first.c_str(), pair.second);
    }

    printf("  ELink Event Occupancy: %d\n", pft->daq().getEventOccupancy());
  }
}

namespace {
auto menu_expert = pftool::menu("EXPERT", "expert functions");
auto menu_i2c = menu_expert->submenu("I2C", "raw I2C interactions")
                    ->line("BUS", "Pick the I2C bus to use", i2c)
                    ->line("READ", "Read from an address", i2c)
                    ->line("WRITE", "Write to an address", i2c)
                    ->line("MULTIREAD", "Read from an address", i2c)
                    ->line("MULTIWRITE", "Write to an address", i2c);
auto menu_elinks =
    menu_expert->submenu("ELINKS", "manage the elinks")
        ->line("RELINK", "Follow standard procedure to establish links", elinks)
        ->line("HARD_RESET", "Hard reset of the PLL", elinks)
        ->line("STATUS", "Elink status summary", elinks)
        ->line("SPY", "Spy on an elink", elinks)
        ->line("AUTO", "Attempt to re-align automatically", elinks)
        ->line("BITSLIP", "Set the bitslip for a link or turn on auto", elinks)
        ->line("SCAN", "Scan on an elink", elinks)
        ->line("DELAY", "Set the delay on an elink", elinks);

auto menu_fc =
    menu_expert
        ->submenu("FAST_CONTROL", "configuration and testing of fast control")
        ->line("STATUS", "Check status and counters", fc)
        ->line("SW_L1A", "Send a software L1A", fc)
        ->line("LINK_RESET", "Send a link reset", fc)
        ->line("BUFFER_CLEAR", "Send a buffer clear", fc)
        ->line("RUN_CLEAR", "Send a run clear", fc)
        ->line("COUNTER_RESET", "Reset counters", fc)
        ->line("CALIB", "Setup calibration pulse", fc)
        ->line("LED", "Setup LED calibration pulse", fc);
}  // namespace
