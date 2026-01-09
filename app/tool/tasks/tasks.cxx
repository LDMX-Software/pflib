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

// 12 bits, default 0x2
static const int ALIGNER_ORBSYN_CNT_SNAPSHOT = 0x0380 + 0x16;

static const int CHALIGNER_BASE[12] = {
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

// 1b, flag if pattern found
static const int CHALIGNER_PATTERN_MATCH = 0x0;

// snapshot location, 192b
static const int CHALIGNER_SNAPSHOT = 0x2;

void scan_l1a_offset(Target* tgt) {
  auto& econ{tgt->econ(0)};
  for (int t_snapshot{3490}; t_snapshot < 3540; t_snapshot+=4) {
    econ.setValue(ALIGNER_ORBSYN_CNT_SNAPSHOT, t_snapshot, 3);
    // pause to make sure new snapshot is taken?
    usleep(1000);

    printf("%d:\n", t_snapshot);
    for (int ch{9}; ch < 11; ch++) {
      std::string channel = std::to_string(ch);
      std::string var_name_pm = channel + "_PATTERN_MATCH";
      auto ch_pm = econ.readParameter("CHALIGNER", var_name_pm);
      bool pattern_match = (ch_pm == 1);

      std::string var_name_snapshot1 = channel + "_SNAPSHOT_0";
      std::string var_name_snapshot2 = channel + "_SNAPSHOT_1";
      std::string var_name_snapshot3 = channel + "_SNAPSHOT_2";
      auto ch_snapshot_1 = econ.readParameter("CHALIGNER", var_name_snapshot1);
      auto ch_snapshot_2 = econ.readParameter("CHALIGNER", var_name_snapshot2);
      auto ch_snapshot_3 = econ.readParameter("CHALIGNER", var_name_snapshot3);
      printf(" %2d: %s %08x %08x %08x\n",
        ch, (pattern_match ? "*": " "),
        ch_snapshot_3, ch_snapshot_2, ch_snapshot_1);

      /*
       * attempting to do a quick mimic failed
      bool pattern_match = ((econ.getValues(CHALIGNER_BASE[ch]+CHALIGNER_PATTERN_MATCH, 1)[0] & 0x1)==1);
      auto snapshot = econ.getValues(CHALIGNER_BASE[ch]+CHALIGNER_SNAPSHOT, 24);

      printf(" %2d: %s", ch, (pattern_match ? "*": " "));
      // snapshot is little-endian
      printf("%2d", snapshot.size());
      for (std::size_t i_byte{snapshot.size()}; i_byte > 0; i_byte--) {
        printf("%02x", static_cast<int>(snapshot[i_byte]));
        if (i_byte % 8 == 0) printf(" ");
      }
      printf("\n");
      */
    }
  }
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
