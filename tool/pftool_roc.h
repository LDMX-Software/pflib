#ifndef PFTOOL_ROC_H
#define PFTOOL_ROC_H
#include "Menu.h"
#include "pflib/PolarfireTarget.h"
#include "pflib/Compile.h"
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
                        const std::string& parameter, const int value, int num_rocs = -1);


void dump_rocconfig(PolarfireTarget* pft, const int iroc);
void load_parameters(PolarfireTarget* pft, const int iroc);

std::string make_roc_config_filename(const int config_version, const int roc);
void load_parameters(PolarfireTarget* pft, const int iroc, const std::string& fname,
                     const bool prepend_defaults, const int num_rocs);
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
int get_dpm_number();
int get_num_channels_per_elink();
int get_num_channels_per_roc();
#endif /* PFTOOL_ROC_H */
