#ifndef PFTOOL_ROC_H
#define PFTOOL_ROC_H

#include "pftool.h"

// Forward declaration due to unorganized source structure
std::string get_output_directory();

using pflib::PolarfireTarget;
/**
 * ROC currently being interacted with by user
 */
extern int iroc;

/**
 * Simply print the currently selective ROC so that user is aware
 * which ROC they are interacting with by default.
 *
 * @param[in] pft active target (not used)
 */
void roc_render( PolarfireTarget* pft );


void poke_all_channels(PolarfireTarget* pft, const std::string& parameter,
                       const int value);

void poke_all_rochalves(PolarfireTarget *pft, const std::string& page_template,
                        const std::string& parameter, const int value);


std::string make_default_rocdump_filename(const int dpm,
                                          const int iroc);
void dump_rocconfig(PolarfireTarget* pft,
                    const int iroc);

std::string make_roc_config_filename(const int config_version, const int roc);
// Ask for everything
void load_parameters(PolarfireTarget* pft, const int iroc);
// Load a particular roc config
void load_parameters(PolarfireTarget* pft, const int iroc, const std::string& fname,
                     const bool prepend_defaults);
// Load multiple files
void load_parameters(PolarfireTarget* pft, const int config_version,
                     const std::vector<std::string> filenames,
                     const bool prepend_values);
/**
 * ROC menu commands
 *
 * When necessary, the ROC interaction object pflib::ROC is created
 * via pflib::Hcal::roc with the currently active roc.
 *
 * ## Commands
 * - HARDRESET : pflib::Hcal::hardResetROCs
 * - SOFTRESET : pflib::Hcal::softResetROC
 * - RESYNCLOAD : pflib::Hcal::resyncLoadROC
 * - IROC : Change which ROC to focus on
 * - CHAN : pflib::ROC::getChannelParameters
 * - PAGE : pflib::ROC::readPage
 * - PARAM_NAMES : Use pflib::parameters to get list ROC parameter names
 * - POKE_REG : pflib::ROC::setValue
 * - POKE_PARAM : pflib::ROC::applyParameter
 * - LOAD_REG : pflib::PolarfireTarget::loadROCRegisters
 * - LOAD_PARAM : pflib::PolarfireTarget::loadROCParameters
 * - DUMP : pflib::PolarfireTarget::dumpSettings
 *
 * @param[in] cmd ROC command
 * @param[in] pft active target
 */
void roc( const std::string& cmd, PolarfireTarget* pft );

int get_num_rocs();
int get_dpm_number(pflib::PolarfireTarget* pft = nullptr);
int get_num_channels_per_elink();
int get_num_channels_per_roc();
int get_number_of_samples_per_event(pflib::PolarfireTarget* pft);
#endif /* PFTOOL_ROC_H */
