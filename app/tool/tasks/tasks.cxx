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
#include "inv_vref_scan.h"
#include "level_pedestals.h"
#include "load_parameter_points.h"
#include "noinv_vref_scan.h"
#include "parameter_timescan.h"
#include "sampling_phase_scan.h"
#include "set_toa.h"
#include "toa_scan.h"
#include "toa_vref_scan.h"
#include "trim_inv_dacb_scan.h"
#include "trim_toa_scan.h"
#include "vt50_scan.h"

namespace {
auto menu_tasks =
    pftool::menu("TASKS",
                 "tasks for studying the chip and tuning its parameters")
        ->line("CHARGE_TIMESCAN", "scan charge/calib pulse over time",
               charge_timescan)
        ->line("GEN_SCAN", "scan over file of input parameter points", gen_scan)
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
        ->line("TRIM_TOA_SCAN",
               "calibrate TRIM_TOA parameters for each channel", trim_toa_scan)
        ->line("PHASE_WORD_ALIGN", "align phase and word", align_phase_word)
        ->line("ALIGN_ECON_LPGBT", "align ECON-D to lpGBT interface",
               align_econ_lpgbt);
}
