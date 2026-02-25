/**
 * @file tasks.cxx
 *
 * Definition of TASKS menu commands
 */
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
#include "sampling_phase_scan.h"
#include "set_toa.h"
#include "toa_scan.h"
#include "toa_vref_scan.h"
#include "tot_vref_scan.h"
#include "trim_inv_dacb_scan.h"
#include "trim_toa_scan.h"
#include "vt50_scan.h"

#include <boost/multiprecision/cpp_int.hpp>

static const int ALIGNER_BASE = 0x0380;

// 12 bits, default 0x2
static const int ALIGNER_ORBSYN_CNT_SNAPSHOT = ALIGNER_BASE + 0x16;

static const int CHALIGNER_RW[12] = {
  0x000,
  0x040,
  0x080,
  0x0c0,
  0x100,
  0x140,
  0x180,
  0x1c0,
  0x200,
  0x240,
  0x280,
  0x2c0
};

static const int CHALIGNER_RO[12] = {
  0x014,
  0x054,
  0x094,
  0x0d4,
  0x114,
  0x154,
  0x194,
  0x1d4,
  0x214,
  0x254,
  0x294,
  0x2d4
};

// 1b, flag if pattern found
static const int CHALIGNER_PATTERN_MATCH = 0x0;

// snapshot location, 192b
static const int CHALIGNER_SNAPSHOT = 0x2;

void scan_l1a_offset(Target* tgt) {
  auto& econ{tgt->econ(0)};
  auto& roc{tgt->roc(0)};

  auto output_filepath{pftool::readline_path("full-orbit-scan-snapshots", ".csv")};
  std::ofstream output{output_filepath};

  /**
   * we want to do manually-triggered I2C snapshots
   * These snapshots will still happen on the ORBSYN_CNT_SNAPTSHOT value
   * within the orbit, but will not only happen after a link reset is sent
   * to the ROC.
   */
  auto manual_snaps_builder = econ.testParameters()
    .add("ALIGNER", "GLOBAL_I2C_SNAPSHOT_EN", 1)
    .add("ALIGNER", "GLOBAL_SNAPSHOT_ARM", 0)
    .add("ALIGNER", "GLOBAL_SNAPSHOT_EN", 1);
  for (int i_econ_ch{0}; i_econ_ch < 12; i_econ_ch++) {
    auto prefix{std::to_string(i_econ_ch)};
    manual_snaps_builder
      .add("CHALIGNER", prefix+"_PER_CH_ALIGN_EN", 0)
      .add("ERX", prefix+"_ENABLE", 1);
  }
  auto manual_snaps = manual_snaps_builder.apply();
  auto original_snapshot_t =
          econ.readParameter("ALIGNER", "GLOBAL_ORBSYN_CNT_SNAPSHOT");

  // start sending an L1A on *every* orbit so that we should have seen
  // one no matter which orbit we end up taking the snapshots in
  bool enable_orbit_blinker{false};
  int bx{0};
  tgt->fc().fc_get_orbit_blinker(enable_orbit_blinker, bx);
  enable_orbit_blinker = pftool::readline_bool("Should the orbit blinker be turned on?", enable_orbit_blinker);
  if (enable_orbit_blinker) {
    int bx = pftool::readline_int("What BX should it blink on?", bx);
    tgt->fc().fc_setup_orbit_blinker(true, bx);
  }

  std::map<int,int> select;
  for (int ch{0}; ch < 12; ch++) {
    select[ch] = econ.getValues(CHALIGNER_RO[ch]+0x1, 1)[0];
  }

  bool show_raw_snapshot = pftool::readline_bool("Show raw snapshot (Y) or shift by align select (N)?", false);

  output << "t_snapshot,00,01,02,03,04,05,06,07,08,09,10,11\n";

  std::vector<uint8_t> aligner_flags(1);
  for (int t_snapshot{0}; t_snapshot < 10 /*3564*/; t_snapshot++) {
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
    usleep(1000);

    output << t_snapshot;
    for (int ch{0}; ch < 12; ch++) {

      boost::multiprecision::uint256_t snapshot{0};
      // have to read in 8byte chunks
      for (int i_snp{0}; i_snp < 3; i_snp++) {
        auto word = econ.getValues(CHALIGNER_RO[ch]+CHALIGNER_SNAPSHOT+8*i_snp, 8);
        for (int i_byte{0}; i_byte < 8; i_byte++) {
          snapshot |= (boost::multiprecision::uint256_t(word[i_byte]) << (8*(8*i_snp + i_byte)));
        }
      }

      if (show_raw_snapshot) {
        output << ',' << std::hex << snapshot << std::dec;
      } else {
        output << ',' << std::hex << ((snapshot >> (select[ch] - 32)) & 0xffffffff) << std::dec;
      }
    }
    output << '\n';
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
        ->line("SCAN_L1A_OFFSET", "scan L1A offset in ECON to try to find data", scan_l1a_offset)
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
               align_econ_lpgbt);
}
