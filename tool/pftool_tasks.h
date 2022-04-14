#ifndef PFTOOL_TASKS_H
#define PFTOOL_TASKS_H

#include "pflib/PolarfireTarget.h"
#include <fstream>
#include "Menu.h"
#include "pflib/decoding/SuperPacket.h"
#include "pftool_bias.h"
#include "pftool_roc.h"
#include "pftool_elinks.h"
#include "pftool_fastcontrol.h"
#include "pftool_daq.h"
#include "pftool_hardcoded_values.h"
/**
 * TASK menu commands
 *
 * ## Commands
 * - RESET_POWERUP: execute a series of commands after chip has been power cycled or firmware re-loaded
 * - SCANCHARGE : loop over channels and scan a range of a CALIBDAC values, recording only relevant channel points in CSV file
 *
 * @param[in] cmd command selected from menu
 * @param[in] pft active target
 */

void tasks( const std::string& cmd, pflib::PolarfireTarget* pft );

void beamprep(pflib::PolarfireTarget* pft);



void set_one_channel_per_elink(PolarfireTarget* pft,
                               const std::string& parameter,
                               const int channels_per_elink,
                               const int ichan,
                               const int value);

void enable_one_channel_per_elink(PolarfireTarget* pft, const std::string& modeinfo,
                                  const int channels_per_elink,
                                  const int ichan);
void disable_one_channel_per_elink(PolarfireTarget* pft, const std::string& modeinfo,
                                   const int channels_per_elink,
                                   const int ichan);
// This screams for RAII...
void teardown_charge_injection(PolarfireTarget* pft);
void prepare_charge_injection(PolarfireTarget* pft);
std::vector<std::string> make_led_filenames();
std::string make_default_led_template();
std::string make_default_chargescan_filename(PolarfireTarget* pft,
                                             const std::string& valuename);
#endif /* PFTOOL_TASKS_H */
