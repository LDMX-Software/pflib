/**
 * @file tasks.cxx
 *
 * Definition of TASKS menu commands
 */

#include "../pftool.h"
#include "channel_wise_calib_scan.h"
#include "charge_timescan.h"
#include "check_pedestal.h"
#include "examine_phase.h"
#include "expert/scan_orbit.h"
#include "gen_scan.h"
#include "get_lpgbt_temps.h"
#include "global_pedestal_level.h"
#include "inv_vref_scan.h"
#include "led_bias_scan.h"
#include "load_parameter_points.h"
#include "local_pedestal_level.h"
#include "multi_channel_scan.h"
#include "noinv_vref_scan.h"
#include "parameter_timescan.h"
#include "pflib/align_econ_lpgbt.h"
#include "pflib/align_econ_rocs.h"
#include "sampling_phase_scan.h"
#include "set_toa.h"
#include "set_zs_thresholds.h"
#include "setup/check_lpgbt_backend.h"
#include "setup/scl_delay_scan.h"
#include "toa_scan.h"
#include "toa_vref_scan.h"
#include "tot_scan.h"
#include "trim_inv_dacb_scan.h"
#include "trim_toa_scan.h"
#include "vref_2d_scan.h"
#include "vt50_scan.h"

namespace {
auto menu_tasks =
    pftool::menu("TASKS",
                 "tasks for studying the chip and tuning its parameters")
        ->line("CHARGE_TIMESCAN", "scan charge/calib pulse over time",
               charge_timescan)
        ->line("GEN_SCAN", "scan over file of input parameter points", gen_scan)
        ->line("GET_LPGBT_TEMPS", "Return the temperatue of the lpGBT",
               get_lpgbt_temps)
        ->line("READ_RTD", "Read a module RTD with the lpGBT current DAC",
               read_rtd)
        ->line("PARAMETER_TIMESCAN",
               "scan charge/calib pulse over time for varying parameters",
               parameter_timescan)
        ->line("CHANNEL_WISE_CALIB_SCAN",
               "scan calib over all channels, one at a time",
               channel_wise_calib_scan)
        ->line("TRIM_INV_DACB_SCAN", "scan trim_inv parameter",
               trim_inv_dacb_scan)
        ->line("INV_VREF_SCAN", "scan over INV_VREF parameter", inv_vref_scan)
        ->line("GLOBAL_PEDESTAL_LEVEL", "Scan over INV_VREF for calibration",
               global_pedestal_level)
        ->line("VREF_2D_SCAN", "scan over INV_VREF and NOINV_VREF",
               vref_2d_scan)
        ->line("NOINV_VREF_SCAN", "scan over NOINV_VREF parameter",
               noinv_vref_scan)
        ->line("EXAMINE_PHASE", "scan over phase parameters", examine_phase)
        ->line("SAMPLING_PHASE_SCAN",
               "scan phase_ck, pedestal for clock phase alignment",
               sampling_phase_scan)
        ->line("MULTI_CHANNEL_SCAN",
               "scans multiple channels to look for cross-talk",
               multi_channel_scan)
        ->line("VT50_SCAN",
               "Hones in on the vt50 with a binary or bisectional scan",
               vt50_scan)
        ->line("LOCAL_PEDESTAL_LEVEL",
               "tune trim_inv and dacb to level pedestals with their link "
               "median",
               local_pedestal_level)
        ->line("TOA_VREF_SCAN", "scan over VREF parameters for TOA calibration",
               toa_vref_scan)
        ->line("TOT_SCAN",
               "scan over VREF and TRIM parameters for TOT calibration",
               tot_scan)
        ->line("TRIM_TOA_SCAN",
               "calibrate TRIM_TOA parameters for each channel", trim_toa_scan)
        ->line("TOA_SCAN", "calibrate TRIM_TOA parameters for each channel",
               toa_scan)
        ->line("LED_BIAS_SCAN", "Sweeps SiPM and LED DACs", led_bias_scan)
        ->line("CHECK_PEDESTAL",
               "compare a pedestal run with the calibration set expectation",
               check_pedestal)
        ->line("SET_ZS_THRESHOLDS",
               "set ECON-D ZS thresholds to pedestal plus a margin",
               set_zs_thresholds);

auto menu_expert_tasks =
    menu_tasks->submenu("EXPERT", "low-level but complicated tasks")
        ->line("SCAN_ORBIT", "scan snapshots in ECON to try to find data",
               scan_orbit);

auto menu_setup_tasks =
    menu_tasks->submenu("SETUP", "tasks when setting up a newly-powered system")
        ->line("SCL_DELAY_SCAN",
               "pick the SCL delay with the fewest ROC I2C errors",
               scl_delay_scan)
        ->line("ALIGN_ECON_ROCS", "align ECON and the ROCs",
               [](Target* tgt) {
                 bool on_zcu = (pftool::state.readout_config() ==
                                pftool::State::CFG_HCALFMC) ||
                               (pftool::state.readout_config() ==
                                pftool::State::CFG_HCALOPTO_ZCU) ||
                               (pftool::state.readout_config() ==
                                pftool::State::CFG_ECALOPTO_ZCU);

                 int iecon = pftool::readline_int("Which ECON to manage: ",
                                                  pftool::state.iecon);

                 pflib::align_econ_rocs(tgt, iecon, on_zcu);
               })
        ->line(
            "ALIGN_ECON_LPGBT", "align ECON and its associated lpGBT",
            [](Target* tgt) {
              bool is_hcal = pftool::state.readout_config_is_hcal();
              int iecon = pftool::readline_int("Which ECON to manage: ",
                                               pftool::state.iecon);
              bool check_all_phases =
                  pftool::readline_bool("Check all phases? ", false);
              if (pftool::readline_bool("Do bit alignment?", true)) {
                pflib::align_econ_lpgbt_bit(tgt, iecon, check_all_phases,
                                            is_hcal);
              }
              if (pftool::readline_bool("Continue to word alignment?", true)) {
                // WARN: assuming ECON-D is index 0 and index 0 is ECON-D
                uint32_t idle_pattern = ((iecon == 0) ? 0x1277cc : 0x526);
                idle_pattern =
                    pftool::readline_int("Idle pattern: ", idle_pattern, true);
                pflib::align_econ_lpgbt_word(tgt, iecon, check_all_phases,
                                             is_hcal, idle_pattern);
              }
            })
        ->line("CHECK_LPGBT_BACKEND", "check lpGBT-Backend data pipeline",
               check_lpgbt_backend);

}  // namespace
