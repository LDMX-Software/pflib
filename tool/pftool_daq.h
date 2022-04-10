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

extern std::string last_run_file;
extern std::string start_dma_cmd;
extern std::string stop_dma_cmd;

/**
 * Print data words from raw binary file and return them
 * @return vector of 32-bit data words in file
 */
std::vector<uint32_t> read_words_from_file();
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
void daq( const std::string& cmd, pflib::PolarfireTarget* pft );

/**
 * DAQ->SETUP menu commands
 *
 * Before doing any of the commands, we retrieve a reference to the daq
 * object via pflib::Hcal::daq.
 *
 * ## Commands
 * - STATUS : pflib::PolarfireTarget::daqStatus
 *   and pflib::rogue::RogueWishboneInterface::daq_dma_status if connected in that manner
 * - ENABLE : toggle whether daq is enabled pflib::DAQ::enable and pflib::DAQ::enabled
 * - ZS : pflib::PolarfireTarget::enableZeroSuppression
 * - L1APARAMS : Use target's wishbone interface to set the L1A delay and capture length
 *   Uses pflib::tgt_DAQ_Inbuffer
 * - DMA : enable DMA readout pflib::rogue::RogueWishboneInterface::daq_dma_enable
 * - FPGA : Set the polarfire FPGA ID number (pflib::DAQ::setIds) and pass this
 *   to DMA setup if it is enabled
 * - STANDARD : Do FPGA command and setup links that are 
 *   labeled as active (pflib::DAQ::setupLink)
 *
 * @param[in] cmd selected command from DAQ->SETUP menu
 * @param[in] pft active target
 */
void daq_setup( const std::string& cmd, pflib::PolarfireTarget* pft );

/**
 * DAQ->DEBUG menu commands
 *
 * @note These commands have been archived since further development of pflib
 * has progressed. They are still available in this submenu; however,
 * they should only be used by an expert who is familiar with the chip
 * and has looked at what the commands do in the code.
 *
 * @param[in] cmd selected command from menu
 * @param[in] pft active target
 */
void daq_debug( const std::string& cmd, pflib::PolarfireTarget* pft );

void setup_dma(pflib::PolarfireTarget* pft);
#endif /* PFTOOL_DAQ_H */
