/**
 * @file tasks.cxx
 *
 * Definition of TASKS menu commands
 */
#include <boost/multiprecision/cpp_int.hpp>

#include "../pftool.h"
#include "align_econ_lpgbt.h"
#include "align_phase_word.h"
#include "charge_timescan.h"
#include "gen_scan.h"
#include "get_lpgbt_temps.h"
#include "inv_vref_scan.h"
#include "level_pedestals.h"
#include "load_parameter_points.h"
#include "multi_channel_scan.h"
#include "noinv_vref_scan.h"
#include "parameter_timescan.h"
#include "pflib/packing/Hex.h"
#include "sampling_phase_scan.h"
#include "set_toa.h"
#include "toa_scan.h"
#include "toa_vref_scan.h"
#include "tot_vref_scan.h"
#include "trim_inv_dacb_scan.h"
#include "trim_toa_scan.h"
#include "vt50_scan.h"
using pflib::packing::hex;

static const int ALIGNER_BASE = 0x0380;

// 12 bits, default 0x2
static const int ALIGNER_ORBSYN_CNT_SNAPSHOT = ALIGNER_BASE + 0x16;

static const int CHALIGNER_RW[12] = {0x000, 0x040, 0x080, 0x0c0, 0x100, 0x140,
                                     0x180, 0x1c0, 0x200, 0x240, 0x280, 0x2c0};

static const int CHALIGNER_RO[12] = {0x014, 0x054, 0x094, 0x0d4, 0x114, 0x154,
                                     0x194, 0x1d4, 0x214, 0x254, 0x294, 0x2d4};

// 1b, flag if pattern found
static const int CHALIGNER_PATTERN_MATCH = 0x0;

// snapshot location, 192b
static const int CHALIGNER_SNAPSHOT = 0x2;

#include <sys/time.h>

void watch_orbit_blinker(Target* tgt) {
  auto& econ{tgt->econ(0)};
  static const int wait_time_us = 10000;
  int bx = pftool::readline_int("What BX should it blink on?", 10);
  bool poke = pftool::readline_bool(
      "Read an ECON parameter while blinker is on?", false);
  std::map<std::string, uint32_t> dbg = tgt->daq().get_debug(0);
  uint32_t starts{dbg.at("COUNT_STARTS")}, stops{dbg.at("COUNT_STOPS")};
  timeval tv0, tvi;
  gettimeofday(&tv0, 0);
  printf("            : t / us : Occu Start Stop\n");
  printf(" Pre-Blinker: %06d : %4d %5d %4d\n", tv0.tv_usec,
         tgt->daq().getEventOccupancy(), starts, stops);
  tgt->fc().fc_setup_orbit_blinker(true, bx);
  if (poke) {
    bool aligner_done = (econ.getValues(0x398, 1)[0] & 0x1);
    printf("Aligner Done: %d\n", aligner_done);
  }
  gettimeofday(&tvi, 0);
  int usec_left = wait_time_us - int(tvi.tv_usec - tv0.tv_usec);
  if (tvi.tv_sec == tv0.tv_sec and usec_left > 100) {
    usleep(usec_left);
  }
  tgt->fc().fc_setup_orbit_blinker(false, bx);
  gettimeofday(&tvi, 0);
  dbg = tgt->daq().get_debug(0);
  starts = dbg.at("COUNT_STARTS");
  stops = dbg.at("COUNT_STOPS");
  printf("Post-Blinker: %06d : %4d %5d %4d\n", tvi.tv_usec,
         tgt->daq().getEventOccupancy(), starts, stops);
}

void scan_l1a_offset(Target* tgt) {
  auto& econ{tgt->econ(0)};
  /**
   * Scan the ECON's L1A timing parameters
   *
   * l1a_to_hgcroc_out: 5bits starting at 0
   * econ_buffer_delay: 5bits starting at 5
   * l1a_offset       : 4bits starting at 10
   */
  static const int ROCDAQCTRL_RW = 0x410;
  static const int ROCDAQCTRL_TIMING = ROCDAQCTRL_RW + 0x3;

  auto timing_bytes = econ.getValues(ROCDAQCTRL_TIMING, 2);
  auto orig_timing_bytes = timing_bytes;
  printf("%02x %02x\n", timing_bytes.at(1), timing_bytes.at(0));
  printf("l1a_to_hgcroc_out: %d\n", (timing_bytes.at(0) & 0x1f));
  printf("econ_buffer_delay: %d\n",
         (timing_bytes.at(0) >> 5) | (timing_bytes.at(1) & 0x3));
  printf("l1a_offset       : %d\n", timing_bytes.at(1) >> 2);

  for (int l1a_diff{0}; l1a_diff < 32; l1a_diff++) {
    timing_bytes[0] |= 0x1f;
    timing_bytes[0] ^= 0x1f;
    timing_bytes[0] |= (l1a_diff & 0x1f);
    econ.setValues(ROCDAQCTRL_TIMING, timing_bytes);

    tgt->fc().sendL1A();
    usleep(100);

    printf("%2d 0x%02x%02x %d\n", l1a_diff, timing_bytes[1], timing_bytes[0],
           tgt->daq().getEventOccupancy());
  }

  econ.setValues(ROCDAQCTRL_TIMING, orig_timing_bytes);
}

void scan_orbit(Target* tgt) {
  /**
   * Take a snapshot on each BX of an orbit while (optionally)
   * sending an L1A once per orbit to see if the words are
   * aligned
   */
  auto& econ{tgt->econ(0)};

  bool to_screen{pftool::readline_bool("Print snapshots to screen?", true)};
  std::ofstream file_output;
  if (not to_screen) {
    auto output_filepath{
        pftool::readline_path("full-orbit-scan-snapshots", ".csv")};
    file_output.open(output_filepath);
  }

  bool show_raw_snapshot = pftool::readline_bool(
      "Show raw snapshot (Y) or shift by align select (N)?", false);
  bool skip_normal_idles =
      pftool::readline_bool("Skip printing normal idles?", true);

  int start_snapshot = pftool::readline_int("First snapshot time?", 0);
  int end_snapshot = pftool::readline_int("Last snapshot time?", 3564);
  std::cout << "daq event occupancy: " << tgt->daq().getEventOccupancy()
            << std::endl;

  /**
   * we want to do manually-triggered I2C snapshots
   * These snapshots will still happen on the ORBSYN_CNT_SNAPTSHOT value
   * within the orbit, but will not only happen after a link reset is sent
   * to the ROC.
   */
  std::map<std::string, std::map<std::string, uint64_t>> manual_snaps;
  manual_snaps["ALIGNER"]["GLOBAL_I2C_SNAPSHOT_EN"] = 1;
  manual_snaps["ALIGNER"]["GLOBAL_SNAPSHOT_ARM"] = 0;
  manual_snaps["ALIGNER"]["GLOBAL_SNAPSHOT_EN"] = 1;
  for (int i_econ_ch{0}; i_econ_ch < 12; i_econ_ch++) {
    auto prefix{std::to_string(i_econ_ch)};
    manual_snaps["CHALIGNER"][prefix + "_PER_CH_ALIGN_EN"] = 0;
    manual_snaps["ERX"][prefix + "_ENABLE"] = 1;
  }
  econ.applyParameters(manual_snaps);
  auto original_snapshot_t =
      econ.readParameter("ALIGNER", "GLOBAL_ORBSYN_CNT_SNAPSHOT");
  std::cout << "daq event occupancy: " << tgt->daq().getEventOccupancy()
            << std::endl;

  std::map<int, int> select;
  std::cout << "select shifts: { ";
  for (int ch{0}; ch < 12; ch++) {
    select[ch] = econ.getValues(CHALIGNER_RO[ch] + 0x1, 1)[0];
    if (ch > 0) {
      std::cout << ", ";
    }
    std::cout << ch << " : " << select[ch];
  }
  std::cout << " }\n";

  // start sending an L1A on *every* orbit so that we should have seen
  // one no matter which orbit we end up taking the snapshots in
  bool enable_orbit_blinker{false};
  int bx{0};
  tgt->fc().fc_get_orbit_blinker(enable_orbit_blinker, bx);
  enable_orbit_blinker = pftool::readline_bool(
      "Should the orbit blinker be turned on?", enable_orbit_blinker);
  if (enable_orbit_blinker) {
    int bx = pftool::readline_int("What BX should it blink on?", bx);
    tgt->fc().fc_setup_orbit_blinker(true, bx);
  }
  std::cout << "daq event occupancy: " << tgt->daq().getEventOccupancy()
            << std::endl;
  usleep(100);
  // std::cout << "ALIGNER.DONE = " << (econ.getValues(0x398, 1)[0] & 0x1) <<
  // std::endl;
  std::cout << "daq event occupancy: " << tgt->daq().getEventOccupancy()
            << std::endl;

  static const char* header{
      "t_snapshot,daq_event_occupancy,00,01,02,03,04,05,06,07,08,09,10,11\n"};
  if (to_screen) {
    std::cout << header;
  } else {
    file_output << header;
  }

  std::vector<uint8_t> aligner_flags(1);
  for (int t_snapshot{start_snapshot}; t_snapshot < end_snapshot;
       t_snapshot++) {
    econ.setValue(ALIGNER_ORBSYN_CNT_SNAPSHOT, t_snapshot, 2);
    aligner_flags = econ.getValues(ALIGNER_BASE, 1);
    // need to make sure that SNAPSHOT_ARM goes from 0 to 1
    // first, clear lowest bit to 0
    aligner_flags[0] &= 0xf6;
    econ.setValues(ALIGNER_BASE, aligner_flags);

    // then, raise lowest bit to 1
    aligner_flags[0] |= 0b1;
    econ.setValues(ALIGNER_BASE, aligner_flags);

    // pause to make sure new snapshot is taken?
    usleep(100);

    std::stringstream row;
    row << std::setw(4) << t_snapshot << ',' << tgt->daq().getEventOccupancy();
    bool should_print{not skip_normal_idles};
    for (int ch{0}; ch < 12; ch++) {
      boost::multiprecision::uint256_t snapshot{0};
      // have to read in 8byte chunks
      for (int i_snp{0}; i_snp < 3; i_snp++) {
        auto word = econ.getValues(
            CHALIGNER_RO[ch] + CHALIGNER_SNAPSHOT + 8 * i_snp, 8);
        for (int i_byte{0}; i_byte < 8; i_byte++) {
          snapshot |= (boost::multiprecision::uint256_t(word[i_byte])
                       << (8 * (8 * i_snp + i_byte)));
        }
      }

      uint32_t val{((snapshot >> (select[ch] - 32)) & 0xffffffff)};
      if (val != 0xaccccccc) {
        should_print = true;
      }

      if (show_raw_snapshot) {
        row << ',' << std::hex << snapshot << std::dec;
      } else {
        row << ',' << hex(val);
      }
    }
    row << '\n';
    if (should_print) {
      if (to_screen) {
        std::cout << row.str();
      } else {
        file_output << row.str();
      }
    }
  }

  if (enable_orbit_blinker) {
    tgt->fc().fc_setup_orbit_blinker(false, bx);
  }
  econ.setValue(ALIGNER_ORBSYN_CNT_SNAPSHOT, original_snapshot_t, 2);
}

namespace {
auto menu_tasks =
    pftool::menu("TASKS",
                 "tasks for studying the chip and tuning its parameters")
        ->line("SCAN_L1A_OFFSET", "scan L1A offset in ECON to try to find data",
               scan_l1a_offset)
        ->line("SCAN_ORBIT", "scan snapshots in ECON to try to find data",
               scan_orbit)
        ->line("CHARGE_TIMESCAN", "scan charge/calib pulse over time",
               charge_timescan)
        ->line("GEN_SCAN", "scan over file of input parameter points", gen_scan)
        ->line("GET_LPGBT_TEMPS", "Return the temperatue of the lpGBT",
               get_lpgbt_temps)
        ->line("PARAMETER_TIMESCAN",
               "scan charge/calib pulse over time for varying parameters",
               parameter_timescan)
        ->line("TRIM_INV_DACB_SCAN", "scan trim_inv parameter",
               trim_inv_dacb_scan)
        ->line("INV_VREF_SCAN", "scan over INV_VREF parameter", inv_vref_scan)
        ->line("NOINV_VREF_SCAN", "scan over NOINV_VREF parameter",
               noinv_vref_scan)
        ->line("SAMPLING_PHASE_SCAN",
               "scan phase_ck, pedestal for clock phase alignment",
               sampling_phase_scan)
        ->line("MULTI_CHANNEL_SCAN",
               "scans multiple channels to look for cross-talk",
               multi_channel_scan)
        ->line("VT50_SCAN",
               "Hones in on the vt50 with a binary or bisectional scan",
               vt50_scan)
        ->line(
            "LEVEL_PEDESTALS",
            "tune trim_inv and dacb to level pedestals with their link median",
            level_pedestals)
        ->line("TOA_VREF_SCAN", "scan over VREF parameters for TOA calibration",
               toa_vref_scan)
        ->line("TOA_SCAN",
               "just does that bro (changes CALIB while saving only TOA)",
               toa_scan)
        ->line("TOT_SCAN",
               "scan over VREF and TRIM parameters for TOT calibration",
               tot_vref_scan)
        ->line("TRIM_TOA_SCAN",
               "calibrate TRIM_TOA parameters for each channel", trim_toa_scan)
        ->line("PHASE_WORD_ALIGN", "align phase and word", align_phase_word)
        ->line("ALIGN_ECON_LPGBT", "align ECON-D to lpGBT interface",
               align_econ_lpgbt)
        ->line("WATCH_ORBIT_BLINKER",
               "enable orbit blinker and watch for events",
               watch_orbit_blinker);
}
