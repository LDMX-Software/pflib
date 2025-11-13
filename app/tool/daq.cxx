/**
 * @file daq.cxx
 * DAQ menu (and submenus) command definitions
 */
#include "daq_run.h"
#include "pflib/packing/Hex.h"
#include "pflib/utility/string_format.h"
#include "pftool.h"

ENABLE_LOGGING();

static void print_daq_status(Target* pft) {
  pflib::DAQ& daq = pft->daq();
  std::cout << "          Enabled: " << std::boolalpha << daq.enabled() << "\n"
            << "          ECON ID: " << daq.econid() << "\n"
            << "  Samples per RoR: " << daq.samples_per_ror() << "\n"
            << "              SoI: " << daq.soi() << "\n"
            << "  Event Occupancy: " << daq.getEventOccupancy() << "\n"
            << std::endl;

  std::map<std::string, uint32_t> dbg;
  dbg = daq.get_debug(0);
  for (auto x : dbg) {
    std::cout << "   " << x.first << ": 0x" << std::hex << x.second << std::dec
              << std::endl;
  }
}

/**
 * DAQ->SETUP menu commands
 *
 * Before doing any of the commands, we retrieve a reference to the daq
 * object via pflib::Hcal::daq.
 *
 * ## Commands
 * - ENABLE : toggle whether daq is enabled pflib::DAQ::enable and
 * pflib::DAQ::enabled
 * - ZS : pflib::Target::enableZeroSuppression
 * - L1APARAMS : Use target's wishbone interface to set the L1A delay and
 * capture length Uses pflib::tgt_DAQ_Inbuffer
 * - FORMAT : Choose the output format to be used (simple HGCROC, ECON, etc)
 * - DMA : enable DMA readout
 * pflib::rogue::RogueWishboneInterface::daq_dma_enable
 * - FPGA : Set the polarfire FPGA ID number (pflib::DAQ::setIds) and pass this
 *   to DMA setup if it is enabled
 *
 * @param[in] cmd selected command from DAQ->SETUP menu
 * @param[in] pft active target
 */
static void daq_setup(const std::string& cmd, Target* pft) {
  pflib::DAQ& daq = pft->daq();
  if (cmd == "ENABLE") {
    daq.enable(!daq.enabled());
  }
  if (cmd == "FORMAT") {
    if (pftool::state.readout_config() == pftool::State::CFG_HCALOPTO) {
      printf("Only acceptable format for now is ECOND_SW_HEADERS\n");
      pftool::state.daq_format_mode = Target::DaqFormat::ECOND_SW_HEADERS;
    } else {
      printf("Format options:\n");
      printf(" (1) ROC with ad-hoc headers as in TB2022\n");
      printf(" (2) ECON with full readout\n");
      // printf(" (3) ECON with ZS\n");
      int i_format = pftool::readline_int(
          " Select one: ", static_cast<int>(pftool::state.daq_format_mode));
      if (i_format < 1 or i_format > 2) {
        PFEXCEPTION_RAISE(
            "BadSel", std::to_string(i_format) + " is not a valid selection.");
      }
      pftool::state.daq_format_mode = static_cast<Target::DaqFormat>(i_format);
    }
  }
  if (cmd == "CONFIG") {
    pftool::state.daq_contrib_id = pftool::readline_int(
        " Contributor id for data: ", pftool::state.daq_contrib_id);
    int econid = pftool::readline_int(" ECON ID: ", daq.econid());
    int samples = 1;  // frozen for now
    int soi = 0;      // frozen for now
    daq.setup(econid, samples, soi);
  }
  /*
  if (cmd=="ZS") {
    int jlink=pftool::readline_int("Which link (-1 for all)? ",-1);
    bool fullSuppress=pftool::readline_bool("Suppress all channels? ",false);
    pft->enableZeroSuppression(jlink,fullSuppress);
  }
  */
  if (cmd == "L1APARAMS") {
    int ilink = pftool::readline_int("Which link? ", -1);
    if (ilink >= 0) {
      int delay, capture;
      daq.getLinkSetup(ilink, delay, capture);
      delay = pftool::readline_int("L1A delay? ", delay);
      capture = pftool::readline_int("L1A capture length? ", capture);
      daq.setupLink(ilink, delay, capture);
    }
  }
  if (cmd == "DMA") {
#ifdef PFTOOL_ROGUE
    auto rwbi = dynamic_cast<pflib::rogue::RogueWishboneInterface*>(pft->wb);
    if (rwbi) {
      bool enabled;
      uint8_t samples_per_event, fpgaid_i;
      rwbi->daq_get_dma_setup(fpgaid_i, samples_per_event, enabled);
      enabled = pftool::readline_bool("Enable DMA? ", enabled);
      rwbi->daq_dma_enable(enabled);
    } else {
      std::cout << "\nNot connected to chip with RogueWishboneInterface, "
                   "cannot activate DMA.\n"
                << std::endl;
    }
#endif
  }
  /*
  if (cmd=="FPGA") {
    int fpgaid=pftool::readline_int("FPGA id: ",daq.getFPGAid());
    daq.setIds(fpgaid);
#ifdef PFTOOL_ROGUE
    auto rwbi=dynamic_cast<pflib::rogue::RogueWishboneInterface*>(pft->wb);
    if (rwbi) {
      bool enabled;
      uint8_t samples_per_event, fpgaid_i;
      rwbi->daq_get_dma_setup(fpgaid_i,samples_per_event, enabled);
      fpgaid_i=(uint8_t(fpgaid));
      rwbi->daq_dma_setup(fpgaid_i,samples_per_event);
    }
#endif
  }
  */
}

/**
 * DAQ.SETUP.STANDARD
 *
 * Do the standard setup for the HGCROC
 * (i.e. the setup everyone should use unless you are
 * looking at changing L1OFFSET or fast control command timing).
 *
 * ## What Do
 * - Setup fast control so that the calib-pulse and l1a are timed in
 *   and the trigger links are delayed slightly so we can gather a
 *   pre-sample (see Issue #140 and PR #151 for context)
 * - Configure ROC to have L1OFFSET corresponding to this FC timing
 * - Put ROC into Running Mode
 * - Set delays and captures on the elinks
 *
 * ## When to Call
 * This command is designed to set up this configuration on other
 * (non software memory) places so that it does not need to be
 * called often. Specifically, we store things in
 * - the firmware's memory block via UIO for link capture and
 *   fast command settings
 * - the HGCROC's parameter registers
 * This means if either the firmware is reset/reloaded or the HGCROC
 * is reset/power-cycled, this function needs to be called.
 *
 * @note If the firmware was changed or the HGCROC was power-cycled,
 * make sure to align the elinks before calling this function
 * (e.g. with EXPERT.ELINKS.AUTO which has been pretty stable).
 */
static void daq_setup_standard(Target* tgt) {
  /// do a standard fast control setup before tuning it below
  tgt->fc().standard_setup();
  if (pftool::state.readout_config() == pftool::State::CFG_HCALOPTO) {
    pftool::state.daq_format_mode = Target::DaqFormat::ECOND_SW_HEADERS;
  }

  if (pftool::state.readout_config() == pftool::State::CFG_HCALFMC) {
    /**
     * In order to be able to shift the trigger link delay
     * away from zero and allow us to capture a pre-sample
     * in the trigger path, we need to bring the L1A closer
     * in time to the injected charge pulse.
     */
    tgt->fc().fc_setup_calib(tgt->fc().fc_get_setup_calib() - 4);
    /// this then requires us to lower the L1OFFSET as well
    std::map<std::string, std::map<std::string, uint64_t>> l1offsets;
    l1offsets["DIGITALHALF_0"]["L1OFFSET"] = 8;
    l1offsets["DIGITALHALF_1"]["L1OFFSET"] = 8;
    /// @note only correct right now for the single-board readout
    auto roc{tgt->roc(pftool::state.iroc)};
    roc.applyParameters(l1offsets);
    roc.setRunMode(true);
    pflib::Elinks& elinks = tgt->elinks();
    auto& daq{tgt->daq()};
    for (int i = 0; i < daq.nlinks(); i++) {
      if (i < 2) {
        /// DAQ link, timed in with pedestals and idles
        daq.setupLink(i, 12, 40);
      } else {
        /**
         * Trigger link, timed in with DAQ.DEBUG.TRIGGER_TIMEIN
         *
         * There is only one word per crossing per trigger link,
         * but we capture four (one pre-sample and 2 following samples).
         */
        daq.setupLink(i, 3, 4);
      }
    }
  }
}

/**
 * DAQ menu commands, DOES NOT include sub-menu commands
 *
 * ## Commands
 * - RESET : pflib::DAQ::reset
 * - READ : pflib::Target::daqReadDirect with option to save output to file
 * - PEDESTAL : pflib::Target::prepareNewRun and then send an L1A trigger with
 *   pflib::Backend::fc_sendL1A and collect events with
 * pflib::Target::daqReadEvent for the input number of events
 * - CHARGE : same as PEDESTAL but using pflib::Backend::fc_calibpulse instead
 * of direct L1A
 * - SCAN : do a PEDESTAL (or CHARGE)-equivalent for each value of
 *   an input parameter with an input min, max, and step
 *
 * @param[in] cmd command selected from menu
 * @param[in] pft active target
 */
static void daq(const std::string& cmd, Target* pft) {
  pflib::DAQ& daq = pft->daq();

  // default is non-DMA readout
  bool dma_enabled = false;

#ifdef PFTOOL_ROGUE
  auto rwbi = dynamic_cast<pflib::rogue::RogueWishboneInterface*>(pft->wb);
  if (rwbi) {
    uint8_t samples_per_event, fpgaid_i;
    rwbi->daq_get_dma_setup(fpgaid_i, samples_per_event, dma_enabled);
  }
#endif
  if (cmd == "RESET") {
    daq.reset();
  }
  if (cmd == "LINK_ECON") {
    pft->fc().linkreset_econs();
  }
  /*
  if (cmd=="EXTERNAL") {
    int run=0;

    FILE* run_no_file=fopen(last_run_file.c_str(),"r");
    if (run_no_file) {
      fscanf(run_no_file,"%d",&run);
      fclose(run_no_file);
      run=run+1;
    }

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    struct tm *gmtm = gmtime(&t);

    run=pftool::readline_int("Run number? ",run);

    char fname_def_format[1024];
    sprintf(fname_def_format,"run%06d_%%Y%%m%%d_%%H%%M%%S.raw",run);
    char fname_def[1024];
    strftime(fname_def, sizeof(fname_def), fname_def_format, tm);

    std::string fname=pftool::readline("Filename :  ", fname_def);

    run_no_file=fopen(last_run_file.c_str(),"w");
    if (run_no_file) {
      fprintf(run_no_file,"%d\n",run);
      fclose(run_no_file);
    }

    FILE* f=0;
    if (!dma_enabled) f=fopen(fname.c_str(),"w");

    int event_target=pftool::readline_int("Target events? ",1000);
    pft->backend->daq_setup_event_tag(run,gmtm->tm_mday,gmtm->tm_mon+1,gmtm->tm_hour,gmtm->tm_min);

    // reset various counters
    pft->prepareNewRun();

    // start DMA, if that's what we're doing...
    if (dma_enabled && !start_dma_cmd.empty()) {
      printf("Launching DMA...\n");
      std::string cmd=start_dma_cmd+" "+fname;
      system(cmd.c_str());
    }

    // enable external triggers
    pft->fc().enables(true,true,false);

    int ievent=0, wasievent=0;
    while (ievent<event_target) {

      if (dma_enabled) {
        int spill,occ,vetoed;
        pft->fc().read_counters(spill,occ,ievent,vetoed);
        if (ievent>wasievent) {
          printf("...Now read %d events\n",ievent);
          wasievent=ievent;
        } else {
          sleep(1);
        }
      } else {
        bool full, empty;
        int samples, esize;
        do {
          pft->backend->daq_status(full, empty, samples, esize);
        } while (empty);
        printf("%d: %d samples waiting for readout...\n",ievent,samples);

        if (f) {
          std::vector<uint32_t> event = pft->daqReadEvent();
          fwrite(&(event[0]),sizeof(uint32_t),event.size(),f);
        }

        ievent++;
      }
    }

    // disable external triggers
    pft->fc().enables(false,true,false);

    if (f) fclose(f);

    if (dma_enabled && !stop_dma_cmd.empty()) {
      printf("Stopping DMA...\n");
      std::string cmd=stop_dma_cmd;
      system(cmd.c_str());
    }

  }
  */
  if (cmd == "PEDESTAL" || cmd == "CHARGE" || cmd == "LED") {
    std::string runname{};
    if (cmd == "PEDESTAL") {
      runname = "pedestal";
    } else if (cmd == "CHARGE") {
      runname = "charge";
    } else if (cmd == "LED") {
      runname = "led";
    }

    int run = pftool::readline_int("Run number? ", run);
    int nevents = pftool::readline_int("How many events? ", 100);
    pftool::state.daq_rate =
        pftool::readline_int("Readout rate? (Hz) ", pftool::state.daq_rate);

    pft->setup_run(run, pftool::state.daq_format_mode,
                   pftool::state.daq_contrib_id);

    std::string fname = pftool::readline_path(runname);
    bool decoding =
        pftool::readline_bool("Should we decode the packet into CSV?", true);

    if (decoding) {
      DecodeAndWriteToCSV writer{all_channels_to_csv(fname + ".csv")};
      daq_run(pft, cmd, writer, nevents, pftool::state.daq_rate);
    } else {
      WriteToBinaryFile writer{fname + ".raw"};
      daq_run(pft, cmd, writer, nevents, pftool::state.daq_rate);
    }
  }
}

/**
 * DAQ.DEBUG.TRIGGER_TIMEIN
 */
static void daq_debug_trigger_timein(Target* tgt) {
  /**
   * This command attempts to deduce the capture delay for the trigger
   * links by taking two runs after setting some parameters on the chip.
   *
   * Assuming the pedestal values on the chip are all ~200 (as is the case
   * at UMN), setting the CH_XX.ADC_PEDESTAL and DIGITALHALF_X.ADC_TH to
   * their maxima (255 and 31 respectively) forces the trigger sums to be
   * zero for pedestals. Including the 4-bit sync header, this means the
   * trigger link zero-word is 0xa0000000.
   */
  static const uint32_t ZERO = 0xa0000000;

  static const uint32_t DAQ_HEADER_PATTERN = 0xf0000005;

  auto& daq{tgt->daq()};
  auto roc{tgt->roc(pftool::state.iroc)};

  pflib_log(info) << "setting up parameters for trigger link testing";

  auto test_param_builder = roc.testParameters();
  for (int ch{0}; ch < 72; ch++) {
    test_param_builder.add(pflib::utility::string_format("CH_%d", ch),
                           "ADC_PEDESTAL", 255);
  }

  for (int half{0}; half < 2; half++) {
    test_param_builder.add(
        pflib::utility::string_format("DIGITALHALF_%d", half), "ADC_TH", 31);
    auto refvol_page{
        pflib::utility::string_format("REFERENCEVOLTAGE_%d", half)};
    test_param_builder.add(refvol_page, "CALIB", 3000);
    test_param_builder.add(refvol_page, "INTCTEST", 1);
  }

  /**
   * We then enable charge injection within certain channels.
   * Each trigger link produces a single 32-bit word cut up into a 4-bit
   * sync header and 4 7-bit trigger sums.
   *
   *   0b1010 | TCX_0 | TCX_1 | TCX_2 | TCX_3
   *
   * We choose channels to inject charge such that each link
   * has a different trigger sum that should be non-zero.
   * - CH_0 -> TC0_0 non-zero
   * - CH_29 -> TC1_2 non-zero
   * - CH_42 -> TC2_1 non-zero
   * - CH_70 -> TC3_3 non-zero
   */
  static const uint32_t TC_0 = 0xafe00000;
  static const uint32_t TC_1 = 0xa01fc000;
  static const uint32_t TC_2 = 0xa0003f80;
  static const uint32_t TC_3 = 0xa000007f;
  std::array<int, 4> injected_channels{0, 29, 42, 70};
  std::array<uint32_t, 4> expected_charge_mask = {TC_0, TC_2, TC_1, TC_3};
  for (const int& ch : injected_channels) {
    test_param_builder.add(pflib::utility::string_format("CH_%d", ch),
                           "LOWRANGE", 1);
  }
  pflib_log(info) << "applying setup parameters";
  auto test_param_handle = test_param_builder.apply();

  do {
    int og_charge_to_l1a = tgt->fc().fc_get_setup_calib();
    int charge_to_l1a =
        pftool::readline_int("Calibration to L1A offset?", og_charge_to_l1a);
    tgt->fc().fc_setup_calib(charge_to_l1a);

    int default_l1offset = 16;
    int l1offset =
        pftool::readline_int("L1Offset on HGCROC?", default_l1offset);
    auto test_l1offset_handle = roc.testParameters()
                                    .add("DIGITALHALF_0", "L1OFFSET", l1offset)
                                    .add("DIGITALHALF_1", "L1OFFSET", l1offset)
                                    .apply();

    int default_global_latency_time = 10;
    int global_latency_time = pftool::readline_int(
        "Global latency time on the HGCROC?", default_global_latency_time);
    auto test_latency_time =
        roc.testParameters()
            .add("MASTERTDC_0", "GLOBAL_LATENCY_TIME", global_latency_time)
            .add("MASTERTDC_1", "GLOBAL_LATENCY_TIME", global_latency_time)
            .apply();

    pflib_log(info) << "storing link settings and expanding capture window";

    /**
     * The window size in the firmware is stored in 6 bits,
     * so the maximum capture window (and therefore maximum delay)
     * is 63 (2^6 - 1).
     *
     * @note Capture windows larger than 63 seem to be naively trimmed
     * without warning or notice.
     */
    int max_delay = 63;
    std::array<int, 6> og_delay{}, og_capture{};
    for (int ilink{0}; ilink < 6; ilink++) {
      daq.getLinkSetup(ilink, og_delay[ilink], og_capture[ilink]);
      daq.setupLink(ilink, 0, max_delay);
    }

    pflib_log(info)
        << "pedestal runs to confirm alignment and trigger-sum suppression";
    tgt->fc().sendL1A();
    usleep(10000);  // one 100Hz cycle later
    std::array<std::vector<uint32_t>, 6> pedestal_link_data;
    for (int ilink{0}; ilink < 6; ilink++) {
      pedestal_link_data[ilink] = daq.getLinkData(ilink);
    }
    tgt->daq().advanceLinkReadPtr();

    pflib_log(info) << "charge injection run to see non-zero trigger sums in "
                       "specific places";
    tgt->fc().chargepulse();
    usleep(10000);  // one 100Hz cycle later
    std::array<std::vector<uint32_t>, 6> charge_link_data;
    for (int ilink{0}; ilink < 6; ilink++) {
      charge_link_data[ilink] = daq.getLinkData(ilink);
    }
    tgt->daq().advanceLinkReadPtr();

    for (int ilink{0}; ilink < 6; ilink++) {
      pflib_log(debug) << "reset link " << ilink << " to delay "
                       << og_delay[ilink] << " and capture "
                       << og_capture[ilink];
      daq.setupLink(ilink, og_delay[ilink], og_capture[ilink]);
    }
    pflib_log(debug) << "reset charge_to_l1a back to " << og_charge_to_l1a;
    tgt->fc().fc_setup_calib(og_charge_to_l1a);

    pflib_log(info) << "analyze words readout from links";
    pflib_log(debug) << "delay : pedestal -> charge";
    std::array<int, 6> delays{-1, -1, -1, -1, -1, -1};
    std::array<std::pair<int, int>, 4> daq_pedestal_charge_adc;
    std::array<std::pair<int, int>, 2> daq_ev;
    for (int ilink{0}; ilink < 6; ilink++) {
      pflib_log(debug) << "Link " << ilink;
      if (ilink < 2) {
        pflib_log(debug) << "DAQ Link " << ilink;
        // daq link analysis
        for (std::size_t i_delay{0}; i_delay < max_delay; i_delay++) {
          uint32_t pedestal{pedestal_link_data.at(ilink).at(i_delay)},
              charge{charge_link_data.at(ilink).at(i_delay)};
          bool is_header{false};
          if ((pedestal & DAQ_HEADER_PATTERN) == DAQ_HEADER_PATTERN and
              (charge & DAQ_HEADER_PATTERN) == DAQ_HEADER_PATTERN) {
            is_header = true;
            delays[ilink] = i_delay;
          }
          pflib_log(debug) << std::setw(2) << i_delay << " : "
                           << pflib::packing::hex(pedestal) << " -> "
                           << pflib::packing::hex(charge)
                           << (is_header ? " <- header" : "");
        }

        if (delays[ilink] < 0) {
          // could not find the header
          continue;
        }

        // check if we found the injected charge pulses
        pflib::packing::DAQLinkFrame pedestal{std::span(
            pedestal_link_data.at(ilink).begin() + delays[ilink], 40)};
        pflib::packing::DAQLinkFrame charge{
            std::span(charge_link_data.at(ilink).begin() + delays[ilink], 40)};
        daq_ev[ilink] = {pedestal.event, charge.event};
        for (std::size_t i_ch{0}; i_ch < injected_channels.size(); i_ch++) {
          const int& ch{injected_channels[i_ch]};
          int i_link = (ch / 36);
          int i_ch_in_link = (ch % 36);
          if (i_link == ilink) {
            daq_pedestal_charge_adc[i_ch] = {
                pedestal.channels[i_ch_in_link].adc(),
                charge.channels[i_ch_in_link].adc()};
          }
        }
      } else {
        // trig link analysis
        const auto& expected_charge{expected_charge_mask.at(ilink - 2)};
        for (std::size_t i_delay{0}; i_delay < max_delay; i_delay++) {
          uint32_t pedestal{pedestal_link_data.at(ilink).at(i_delay)},
              charge{charge_link_data.at(ilink).at(i_delay)};
          /**
           * The pedestal run producing trigger-zero words filters out words
           * that can be captured by this link but "belong" to a different link.
           * We can then check which words are different between the charge and
           * pedestal runs, printing the word indices (delays) for them.
           * The last step is checking if the word from the charge run is zero
           * everywhere except the expected bits.
           */
          bool match_expected = false;
          if (pedestal == ZERO and pedestal != charge and
              (charge & ZERO) == ZERO) {
            match_expected = ((charge & ~expected_charge) == 0);
            if (match_expected) delays[ilink] = static_cast<int>(i_delay);
          }
          pflib_log(debug) << std::setw(2) << i_delay << " : "
                           << pflib::packing::hex(pedestal) << " -> "
                           << pflib::packing::hex(charge)
                           << (match_expected ? "(expected)" : "");
        }
      }
    }

    /**
     * Finally, report the delays where we found the expected bits to be
     * non-zero
     */
    std::cout << "Link : Delay\n";
    for (std::size_t ilink{0}; ilink < 6; ilink++) {
      std::cout << "   " << ilink << " : ";
      if (delays.at(ilink) < 0) {
        std::cout << "not found";
      } else {
        std::cout << delays.at(ilink);
      }
      if (ilink < 2) {
        const auto& [evp, evc] = daq_ev.at(ilink);
        std::cout << " event: " << evp << " -> " << evc;
        for (std::size_t i_ch{2 * ilink}; i_ch < 2 * ilink + 2; i_ch++) {
          const auto& [p, c] = daq_pedestal_charge_adc.at(i_ch);
          const auto& ch = injected_channels.at(i_ch);
          std::cout << " ch_" << ch << (ch < 10 ? " " : "") << ": " << p
                    << " -> " << c;
        }
      }
      std::cout << '\n';
    }
    std::cout << std::flush;
  } while (pftool::readline_bool(
      "Want to try another set of timing parameters?", false));
}

namespace {

auto menu_daq =
    pftool::menu("DAQ", "Data AcQuisition configuration and testing")
        ->line("STATUS", "Status of the DAQ", print_daq_status)
        ->line("RESET", "Reset the DAQ", daq)
        ->line("LINK_ECON", "Reset ECON links", daq)
        ->line("PEDESTAL", "Take a simple random pedestal run", daq)
        ->line("CHARGE", "Take a charge-injection run", daq);

auto menu_daq_debug =
    menu_daq->submenu("DEBUG", "expert functions for debugging DAQ")
        ->line("STATUS", "Provide the status", print_daq_status)
        ->line("ESPY", "Spy on one elink",
               [](Target* tgt) {
                 static int input = 0;
                 input = pftool::readline_int("Which input?", input);
                 pflib::DAQ& daq = tgt->daq();

                 std::vector<uint32_t> buffer = daq.getLinkData(input);
                 int delay{}, capture{};
                 daq.getLinkSetup(input, delay, capture);
                 for (size_t i = 0; i < buffer.size(); i++) {
                   if (i == 0) {
                     printf(" %04d %08x <- %d delay\n", int(i), buffer[i],
                            delay);
                   } else {
                     printf(" %04d %08x\n", int(i), buffer[i]);
                   }
                 }
               })
        ->line("ADV", "advance the readout pointers",
               [](Target* tgt) { tgt->daq().advanceLinkReadPtr(); })
        ->line("SW_L1A", "send a L1A from software",
               [](Target* tgt) { tgt->fc().sendL1A(); })
        ->line(
            "CHARGE_TIMEIN",
            "Scan pulse-l1a time offset to see when it should be",
            [](Target* tgt) {
              int nevents = pftool::readline_int(
                  "How many events per time offset? ", 100);
              int calib = pftool::readline_int(
                  "Setting for calib pulse amplitude? ", 1024);
              int min_offset =
                  pftool::readline_int("Minimum time offset to test? ", 0);
              int max_offset =
                  pftool::readline_int("Maximum time offset to test? ", 128);
              std::string fname = pftool::readline_path("charge-timein");
              tgt->setup_run(1, Target::DaqFormat::SIMPLEROC,
                             pftool::state.daq_contrib_id);
              DecodeAndWriteToCSV writer{all_channels_to_csv(fname + ".csv")};
              pflib::ROC roc{tgt->roc(pftool::state.iroc)};
              auto test_param_handle =
                  roc.testParameters()
                      .add("REFERENCEVOLTAGE_1", "CALIB", calib)
                      .add("REFERENCEVOLTAGE_1", "INTCTEST", 1)
                      .add("CH_61", "HIGHRANGE", 0)
                      .add("CH_61", "LOWRANGE", 0)
                      .apply();
              for (int toffset{min_offset}; toffset < max_offset; toffset++) {
                tgt->fc().fc_setup_calib(toffset);
                usleep(10);
                pflib_log(info) << "run with FAST_CONTROL.CALIB = "
                                << tgt->fc().fc_get_setup_calib();
                daq_run(tgt, "CHARGE", writer, nevents, pftool::state.daq_rate);
              }
            })
        ->line("CHARGE_L1A", "send a charge pulse followed by L1A",
               [](Target* tgt) { tgt->fc().chargepulse(); })
        ->line("L1APARAMS", "setup parameters for L1A capture", daq_setup)
        ->line("TRIGGER_TIMEIN", "look for candidate trigger delays",
               daq_debug_trigger_timein);

auto menu_daq_setup =
    menu_daq->submenu("SETUP", "setup the DAQ")
        ->line("STATUS", "Status of the DAQ", print_daq_status)
        ->line("ENABLE", "Toggle enable status", daq_setup)
        ->line("L1APARAMS", "Setup parameters for L1A capture", daq_setup)
        ->line("STANDARD", "Do the standard setup for HCAL", daq_setup_standard)
        ->line("FORMAT", "Select the output data format", daq_setup)
        ->line("CONFIG", "Setup ECON id, contrib id, samples", daq_setup);

}  // namespace
