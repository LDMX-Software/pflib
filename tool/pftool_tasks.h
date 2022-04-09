#ifndef PFTOOL_TASKS_H
#define PFTOOL_TASKS_H

#include "pflib/PolarfireTarget.h"
#include <fstream>
#include "Menu.h"
#include "pflib/decoding/SuperPacket.h"
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
#endif /* PFTOOL_TASKS_H */
