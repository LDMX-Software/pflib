#ifndef PFTOOL_DAQ_H
#define PFTOOL_DAQ_H
#include "Menu.h"
#include "pflib/PolarfireTarget.h"
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#ifdef PFTOOL_ROGUE
#include "pflib/rogue/RogueWishboneInterface.h"
#endif
#ifdef PFTOOL_UHAL
#include "pflib/uhal/uhalWishboneInterface.h"
#endif
using pflib::PolarfireTarget;

extern std::string last_run_file;
extern std::string start_dma_cmd;
extern std::string stop_dma_cmd;

/**
 * DAQ menu commands, DOES NOT include sub-menu commands
 *
 * ## Commands
 * - STATUS : pflib::PolarfireTarget::daqStatus
 *   and pflib::rogue::RogueWishboneInterface::daq_dma_status if connected in that manner
 * - RESET : pflib::PolarfireTarget::daqSoftReset
 * - HARD_RESET : pflib::PolarfireTarget::daqHardReset
 * - READ : pflib::PolarfireTarget::daqReadDirect with option to save output to file
 * - PEDESTAL : pflib::PolarfireTarget::prepareNewRun and then send an L1A trigger with
 *   pflib::Backend::fc_sendL1A and collect events with pflib::PolarfireTarget::daqReadEvent
 *   for the input number of events
 * - CHARGE : same as PEDESTAL but using pflib::Backend::fc_calibpulse instead of direct L1A
 * - SCAN : do a PEDESTAL (or CHARGE)-equivalent for each value of
 *   an input parameter with an input min, max, and step
 *
 * @param[in] cmd command selected from menu
 * @param[in] pft active target
 */
void daq( const std::string& cmd, PolarfireTarget* pft );


#endif /* PFTOOL_DAQ_H */
