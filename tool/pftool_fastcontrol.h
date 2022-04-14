#ifndef PFTOOL_FASTCONTROL_H
#define PFTOOL_FASTCONTROL_H
#include "pflib/PolarfireTarget.h"
#include "Menu.h"

#ifdef PFTOOL_ROGUE
#include "pflib/rogue/RogueWishboneInterface.h"
#endif
#ifdef PFTOOL_UHAL
#include "pflib/uhal/uhalWishboneInterface.h"
#endif
/**
 * Fast Control (FC) menu commands
 *
 * ## Commands
 * - SW_L1A : pflib::Backend::fc_sendL1A
 * - LINK_RESET : pflib::Backend::fc_linkreset
 * - BUFFER_CLEAR : pflib::Backend::fc_bufferclear
 * - COUNTER_RESET : pflib::FastControl::resetCounters
 * - FC_RESET : pflib::FastControl::resetTransmitter
 * - CALIB : pflib::Backend::fc_setup_calib
 * - MULTISAMPLE : pflib::FastControl::setupMultisample
 *   and pflib::rouge::RogueWishboneInterface::daq_dma_setup if that is how the user is connected
 * - STATUS : print mutlisample status and Error and Command counters
 *   pflib::FastControl::getErrorCounters, pflib::FastControl::getCmdCounters,
 *   pflib::FastControl::getMultisampleSetup
 *
 * @param[in] cmd FC menu command
 * @param[in] pft active target
 */
void fc( const std::string& cmd, pflib::PolarfireTarget* pft);



void veto_setup(pflib::PolarfireTarget* pft,
                bool veto_daq_busy = true,
                bool veto_l1_occ = false,
                bool ask = true);
void fc_calib(pflib::PolarfireTarget* pft,
              const int len, const int offset) ;
void fc_calib(pflib::PolarfireTarget* pft);

#endif /* PFTOOL_FASTCONTROL_H */
