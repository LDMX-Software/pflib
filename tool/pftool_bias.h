#ifndef PFTOOL_BIAS_H
#define PFTOOL_BIAS_H

#include "pflib/PolarfireTarget.h"
#include "Menu.h"

/**
 * BIAS menu commands
 *
 * @note This menu has not been explored thoroughly so some of these commands
 * are not fully developed.
 *
 * ## Commands
 * - STATUS : _does nothing_ after selecting a board.
 * - INIT : pflib::Bias::initialize the selected board
 * - SET : pflib::PolarfireTarget::setBiasSetting
 * - LOAD : pflib::PolarfireTarget::loadBiasSettings
 *
 * @param[in] cmd selected menu command
 * @param[in] pft active target
 */
void bias( const std::string& cmd, pflib::PolarfireTarget* pft );

void set_bias_on_all_connectors(pflib::PolarfireTarget* pft);

#endif /* PFTOOL_BIAS_H */
