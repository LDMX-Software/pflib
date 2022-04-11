#ifndef PFTOOL_ELINKS_H
#define PFTOOL_ELINKS_H

#include "pflib/PolarfireTarget.h"
#include "Menu.h"
#include "pflib/decoding/SuperPacket.h"


/**
 * ELINKS menu commands
 *
 * We retrieve a reference to the current elinks object via
 * pflib::Hcal::elinks and then procede to the commands.
 *
 * ## Commands
 * - RELINK : pflib::PolarfireTarget::elink_relink
 * - SPY : pflib::Elinks::spy
 * - HEADER_CHECK : do a pedestal run and count good/bad headers in it
 * - BITSLIP : pflib::Elinks::setBitslip and pflib::Elinks::setBitslipAuto
 * - BIGSPY : PolarfireTarget::elinksBigSpy
 * - DELAY : pflib::Elinks::setDelay
 * - HARD_RESET : pflib::Elinks::resetHard
 * - SCAN : pflib::Elinks::scanAlign
 * - STATUS : pflib::PolarfireTarget::elinkStatus with std::cout input
 *
 * @param[in] cmd ELINKS command
 * @param[in] pft active target
 */
void elinks( const std::string& cmd, pflib::PolarfireTarget* pft);

void align_elinks(pflib::PolarfireTarget* pft);

#endif /* PFTOOL_ELINKS_H */
