#ifndef PFTOOL_BIAS_H
#define PFTOOL_BIAS_H

#include "pflib/PolarfireTarget.h"
#include "Menu.h"
#include "pftool_roc.h"
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

void set_bias_on_all_active_boards(pflib::PolarfireTarget* pft);
void set_bias_on_all_active_boards(pflib::PolarfireTarget* pft,
                                   const bool set_led,
                                   const int dac_value);
void set_bias_on_all_boards(pflib::PolarfireTarget* pft,
                            const int num_boards,
                            const bool set_led,
                            const int dac_value);
void set_bias_on_all_boards(pflib::PolarfireTarget* pft);

void initialize_bias_on_all_boards(pflib::PolarfireTarget* pft,
                                   const int num_boards = 3);
void initialize_bias(pflib::PolarfireTarget* pft,
                     const int iboard);
#endif /* PFTOOL_BIAS_H */
