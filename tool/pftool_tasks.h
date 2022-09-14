#ifndef PFTOOL_TASKS_H
#define PFTOOL_TASKS_H

#include "pftool.h"

#include "pflib/decoding/SuperPacket.h"
#include "pftool_bias.h"
#include "pftool_roc.h"
#include "pftool_elinks.h"
#include "pftool_fastcontrol.h"
#include "pftool_daq.h"
#include "pftool_hardcoded_values.h"
#include <sys/stat.h>
#include <ctime>
#include <algorithm>
#include <numeric>
#include <cmath>

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

void tot_tune(pflib::PolarfireTarget* pft);

void make_scan_csv_header(PolarfireTarget* pft,
                          std::ofstream& csv_out,
                          const std::string& valuename);



void set_one_channel_per_elink(PolarfireTarget* pft,
                               const std::string& parameter,
                               const int channels_per_elink,
                               const int ichan,
// void take_N_calibevents_with_channel(PolarfireTarget *pft,
//                                      std::ofstream &csv_out,
//                                      const int SiPM_bias,
//                                      const int capacitor_type,
//                                      const int events_per_step, const int
//                                      ichan, const int value);
                               const int value);

void enable_one_channel_per_elink(PolarfireTarget* pft, const std::string& modeinfo,
                                  const int channels_per_elink,
                                  const int ichan);
void disable_one_channel_per_elink(PolarfireTarget* pft, const std::string& modeinfo,
                                   const int channels_per_elink,
                                   const int ichan);

// void scan_N_steps(PolarfireTarget *pft, std::ofstream &csv_out,
//                   const int SiPM_bias, const int events_per_step,
//                   const int steps, const int low_value, const int high_value,
//                   const std::string &valuename, const std::string
//                   &pagetemplate, const std::string &modeinfo);
// This screams for RAII...
void teardown_charge_injection(PolarfireTarget* pft);
void prepare_charge_injection(PolarfireTarget* pft);


void calibrun_ledruns(pflib::PolarfireTarget* pft,
                      const std::vector<std::string>& led_filenames);
void calibrun(pflib::PolarfireTarget* pft,
             const std::string& pedestal_filename,
             const std::string& chargescan_filename,
              const std::vector<std::string>& led_filenames);

std::vector<std::string> make_led_filenames();
std::string make_default_led_template();
std::string make_default_chargescan_filename(PolarfireTarget* pft,
                                             const int dpm,
                                             const std::string& valuename,
                                             const int calib_offset = -1);
bool directory_exists(const std::string& directory);
std::string get_yearmonthday();
std::string get_output_directory();


double get_average_adc(pflib::PolarfireTarget* pft,
                       const pflib::decoding::SuperPacket& data,
                       const int link,
                       const int ch);

std::vector<double> get_pedestal_stats(pflib::PolarfireTarget*pft,
                                       const pflib::decoding::SuperPacket& data,
                                       const int link);

std::vector<double> get_pedestal_stats(pflib::PolarfireTarget* pft);
void test_dacb_one_channel_at_a_time(pflib::PolarfireTarget* pft);
void read_charge(PolarfireTarget* pft);
void read_pedestal(PolarfireTarget* pft);
std::vector<double> read_samples(PolarfireTarget *pft,
                                 const pflib::decoding::SuperPacket &data);
#endif /* PFTOOL_TASKS_H */
