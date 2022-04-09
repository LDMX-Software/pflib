#include "pftool.h"

/**
 * @file pftool.cc File defining the pftool menus and their commands.
 *
 * The commands are written into functions corresponding to the menu's name.
 */

/**
 * pull the target of our menu into this source file to reduce code
 */
using pflib::PolarfireTarget;
/**
 * Main status of menu
 *
 * Prints the firmware version and which links are labeled as active.
 *
 * @param[in] pft pointer to active target
 */
static void status( PolarfireTarget* pft ) {
  std::pair<int,int> version = pft->getFirmwareVersion();
  printf(" Polarfire firmware : %4x.%02x\n",version.first,version.second);
  printf("  Active DAQ links: ");
  for (int ilink=0; ilink<pft->hcal.elinks().nlinks(); ilink++)
    if (pft->hcal.elinks().isActive(ilink)) printf("%d ",ilink);
  printf("\n");
}



/**
 * LINK menu commands
 *
 * @note All interaction with target has been commented out.
 * These commands do nothing at the moment.
 *
 * @param[in] cmd LINK command
 * @param[in] pft active target
 */
static void link( const std::string& cmd, PolarfireTarget* pft ) {
  if (cmd=="STATUS") {
    uint32_t status;
    uint32_t ratetx=0, raterx=0, ratek=0;
    //    uint32_t status=ldmx->link_status(&ratetx,&raterx,&ratek);
    printf("Link Status: %08x   TX Rate: %d   RX Rate: %d   K Rate : %d\n",status,ratetx,raterx,ratek);
  }
  if (cmd=="CONFIG") {
    bool reset_txpll=BaseMenu::readline_bool("TX-PLL Reset?", false);
    bool reset_gtxtx=BaseMenu::readline_bool("GTX-TX Reset?", false);
    bool reset_tx=BaseMenu::readline_bool("TX Reset?", false);
    bool reset_rx=BaseMenu::readline_bool("RX Reset?", false);
    static bool polarity=false;
    polarity=BaseMenu::readline_bool("Choose negative TX polarity?",polarity);
    static bool polarity2=false;
    polarity2=BaseMenu::readline_bool("Choose negative RX polarity?",polarity2);
    static bool bypass=false;
    bypass=BaseMenu::readline_bool("Bypass CRC on receiver?",false);
    //    ldmx->link_setup(polarity,polarity2,bypass);
    // ldmx->link_resets(reset_txpll,reset_gtxtx,reset_tx,reset_rx);
  }
  if (cmd=="SPY") {
    /*
    std::vector<uint32_t> spy=ldmx->link_spy();
    for (size_t i=0; i<spy.size(); i++)
      printf("%02d %05x\n",int(i),spy[i]);
    */
  }
}

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
static void elinks( const std::string& cmd, PolarfireTarget* pft ) {
  pflib::Elinks& elinks=pft->hcal.elinks();
  static int ilink=0,nevents{100};
  static int min_delay{0}, max_delay{128};
  if (cmd=="RELINK"){
    ilink=BaseMenu::readline_int("Which elink? (-1 for all) ",ilink);
    min_delay=BaseMenu::readline_int("Min delay? ",min_delay);
    max_delay=BaseMenu::readline_int("Max delay? ",max_delay);
    pft->elink_relink(ilink,min_delay,max_delay);
  }
  if (cmd=="SPY") {
    ilink=BaseMenu::readline_int("Which elink? ",ilink);
    std::vector<uint8_t> spy=elinks.spy(ilink);
    for (size_t i=0; i<spy.size(); i++)
      printf("%02d %05x\n",int(i),spy[i]);
  }
  if (cmd=="HEADER_CHECK") {
    nevents=BaseMenu::readline_int("Num events? ",nevents);

    int nsamples=1;
    {
      bool multi;
      int nextra;
      pft->hcal.fc().getMultisampleSetup(multi,nextra);
      if (multi) nsamples=nextra+1;
    }
  
    pft->prepareNewRun();
  
    int n_good_bxheaders[8] = {0,0,0,0,0,0,0,0};
    int n_bad_bxheaders[8] = {0,0,0,0,0,0,0,0};
    int n_good_idles[8] = {0,0,0,0,0,0,0,0};
    int n_bad_idles[8] = {0,0,0,0,0,0,0,0};
    for (int ievt{0}; ievt < nevents; ievt++) {
      pft->backend->fc_sendL1A();
      std::vector<uint32_t> event_raw = pft->daqReadEvent();
      pflib::decoding::SuperPacket event{&(event_raw[0]), int(event_raw.size())};
      for (int s{0}; s < nsamples; s++) {
        for (int l{0}; l < 8; l++) {
          auto packet = event.sample(s).roc(l);
          if (packet.length() > 2) {
            if (event.sample(s).roc(l).good_bxheader()) n_good_bxheaders[l]++;
            else n_bad_bxheaders[l]++;
            if (event.sample(s).roc(l).good_idle()) n_good_idles[l]++;
            else n_bad_idles[l]++;
          }
        }
      }
    }

    printf("     %26s | %26s\n","BX Headers","Idles");
    printf("Link %10s %10s %4s | %10s %10s %4s\n","Good","Bad","B/T","Good","Bad","B/T");
    for (int ilink{0}; ilink < 8; ilink++) {
      float bg_bxheaders = 0.;
      if (n_good_bxheaders[ilink]+n_bad_bxheaders[ilink]>0) 
        bg_bxheaders = float(n_bad_bxheaders[ilink])/
          (n_good_bxheaders[ilink]+n_bad_bxheaders[ilink]);
      float bg_idles = 0.;
      if (n_good_idles[ilink]+n_bad_idles[ilink]>0) 
        bg_idles = float(n_bad_idles[ilink])/
          (n_good_idles[ilink]+n_bad_idles[ilink]);
      printf("%4d %10d %10d %.2f | %10d %10d %.2f\n", ilink, 
          n_good_bxheaders[ilink], n_bad_bxheaders[ilink], bg_bxheaders,
          n_good_idles[ilink], n_bad_idles[ilink], bg_idles);
    }
  }
  if (cmd=="BITSLIP") {
    ilink=BaseMenu::readline_int("Which elink? ",ilink);
    int bitslip=BaseMenu::readline_int("Bitslip value (-1 for auto): ",elinks.getBitslip(ilink));
    for (int jlink=0; jlink<8; jlink++) {
      if (ilink>=0 && jlink!=ilink) continue;
      if (bitslip<0) elinks.setBitslipAuto(jlink,true);
      else {
        elinks.setBitslipAuto(jlink,false);
        elinks.setBitslip(jlink,bitslip);
      }
    }
  }
  if (cmd=="BIGSPY") {
    int mode=BaseMenu::readline_int("Mode? (0=immediate, 1=L1A) ",0);
    ilink=BaseMenu::readline_int("Which elink? ",ilink);
    int presamples=BaseMenu::readline_int("Presamples? ",20);
    std::vector<uint32_t> words = pft->elinksBigSpy(ilink,presamples,mode==1);
    for (int i=0; i<presamples+100; i++) {
      printf("%03d %08x\n",i,words[i]);
    }
  }
  if (cmd=="DELAY") {
    ilink=BaseMenu::readline_int("Which elink? ",ilink);
    int idelay=BaseMenu::readline_int("Delay value: ",128);
    elinks.setDelay(ilink,idelay);
  }
  if (cmd=="HARD_RESET") {
    elinks.resetHard();
  }
  if (cmd=="SCAN") {
    ilink=BaseMenu::readline_int("Which elink? ",ilink);
    elinks.scanAlign(ilink);
  }
  if (cmd=="STATUS") {
    pft->elinkStatus(std::cout);
  } 
}


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
static void fc( const std::string& cmd, PolarfireTarget* pft ) {
  bool do_status=false;

  if (cmd=="SW_L1A") {
    pft->backend->fc_sendL1A();
    printf("Sent SW L1A\n");
  }
  if (cmd=="LINK_RESET") {
    pft->backend->fc_linkreset();
    printf("Sent LINK RESET\n");
  }
  if (cmd=="RUN_CLEAR") {
    pft->backend->fc_clear_run();
    std::cout << "Cleared run counters" << std::endl;
  }
  if (cmd=="BUFFER_CLEAR") {
    pft->backend->fc_bufferclear();
    printf("Sent BUFFER CLEAR\n");
  }
  if (cmd=="VETO_SETUP") {
    bool veto_daq_busy, veto_l1_occ;
    int level_busy, level_ok;
    pft->backend->fc_veto_setup_read(veto_daq_busy, veto_l1_occ,level_busy, level_ok);
    veto_daq_busy=BaseMenu::readline_bool("Veto L1A on DAQ busy? ",veto_daq_busy);
    veto_l1_occ=BaseMenu::readline_bool("Veto L1A on L1 occupancy? ",veto_l1_occ);    
    pft->backend->fc_veto_setup(veto_daq_busy, veto_l1_occ,level_busy, level_ok);
    if (veto_l1_occ) printf("\n  Occupancy Veto Thresholds -- OK->BUSY at %d, BUSY->OK at %d\n",level_busy,level_ok);
  }
  if (cmd=="COUNTER_RESET") {
    pft->hcal.fc().resetCounters();
    do_status=true;
  }
  if (cmd=="FC_RESET") {
    pft->hcal.fc().resetTransmitter();
  }
  if (cmd=="CALIB") {
    int len, offset;
    pft->backend->fc_get_setup_calib(len,offset);
#ifdef PFTOOL_UHAL
    std::cout <<
      "NOTE: A known bug in uMNio firmware which has been patched in later versions\n"
      "      leads to the inability of the firmware to read some parameters.\n"
      "      If you are seeing 0 as the default even after setting these parameters,\n"
      "      you have this (slightly) buggy firmware."
      << std::endl << std::endl;
#endif
    len=BaseMenu::readline_int("Calibration pulse length?",len);
    offset=BaseMenu::readline_int("Calibration L1A offset?",offset);
    pft->backend->fc_setup_calib(len,offset);
  }
  if (cmd=="MULTISAMPLE") {
    bool multi;
    int nextra;
    pft->hcal.fc().getMultisampleSetup(multi,nextra);
    multi=BaseMenu::readline_bool("Enable multisample readout? ",multi);
    if (multi)
      nextra=BaseMenu::readline_int("Extra samples (total is +1 from this number) : ",nextra);
    pft->hcal.fc().setupMultisample(multi,nextra);
    // also, DMA needs to know
#ifdef PFTOOL_ROGUE
    auto rwbi=dynamic_cast<pflib::rogue::RogueWishboneInterface*>(pft->wb);
    if (rwbi) {
      bool enabled;
      uint8_t samples_per_event, fpgaid_i;
      rwbi->daq_get_dma_setup(fpgaid_i,samples_per_event, enabled);
      samples_per_event=nextra+1;
      rwbi->daq_dma_setup(fpgaid_i,samples_per_event);
    }
#endif
  }
  if (cmd=="STATUS" || do_status) {
    static const std::vector<std::string> bit_comments = {
      "orbit requests",
      "l1a/read requests",
      "",
      "",
      "",
      "calib pulse requests",
      "",
      ""
    };
    bool multi;
    int nextra;
    pft->hcal.fc().getMultisampleSetup(multi,nextra);
    if (multi) printf(" Multisample readout enabled : %d extra L1a (%d total samples)\n",nextra,nextra+1);
    else printf(" Multisaple readout disabled\n");
    printf(" Snapshot: %03x\n",pft->wb->wb_read(1,3));
    uint32_t sbe,dbe;
    pft->hcal.fc().getErrorCounters(sbe,dbe);
    printf("  Single bit errors: %d     Double bit errors: %d\n",sbe,dbe);
    std::vector<uint32_t> cnt=pft->hcal.fc().getCmdCounters();
    for (int i=0; i<8; i++) 
      printf("  Bit %d count: %20u (%s)\n",i,cnt[i],bit_comments.at(i).c_str()); 
    int spill_count, header_occ, header_occ_max, event_count,vetoed_counter;
    pft->backend->fc_read_counters(spill_count, header_occ, header_occ_max, event_count, vetoed_counter);
    printf(" Spills: %d  Events: %d  Header occupancy: %d (max %d)  Vetoed L1A: %d\n",spill_count,event_count,header_occ,header_occ_max,vetoed_counter);
  }
  if (cmd=="ENABLES") {
    bool ext_l1a, ext_spill, timer_l1a;
    pft->backend->fc_enables_read(ext_l1a, ext_spill, timer_l1a);
    ext_l1a=BaseMenu::readline_bool("Enable external L1A? ",ext_l1a);
    ext_spill=BaseMenu::readline_bool("Enable external spill? ",ext_spill);
    timer_l1a=BaseMenu::readline_bool("Enable timer L1A? ",timer_l1a);
    pft->backend->fc_enables(ext_l1a, ext_spill, timer_l1a);
  }
}

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
static void daq_setup( const std::string& cmd, PolarfireTarget* pft ) {
  pflib::DAQ& daq=pft->hcal.daq();
  if (cmd=="STATUS") {
    pft->daqStatus(std::cout);
#ifdef PFTOOL_ROGUE
    auto rwbi=dynamic_cast<pflib::rogue::RogueWishboneInterface*>(pft->wb);
    if (rwbi) {
      bool enabled;
      uint8_t samples_per_event, fpgaid_i;
      rwbi->daq_get_dma_setup(fpgaid_i,samples_per_event, enabled);
      printf("DMA : %s Status=%08x\n",(enabled)?("ENABLED"):("DISABLED"),rwbi->daq_dma_status());
    } 
#endif
  }
  if (cmd=="ENABLE") {
    daq.enable(!daq.enabled());
  }
  if (cmd=="ZS") {
    int jlink=BaseMenu::readline_int("Which link (-1 for all)? ",-1);    
    bool fullSuppress=BaseMenu::readline_bool("Suppress all channels? ",false);
    pft->enableZeroSuppression(jlink,fullSuppress);
  }
  if (cmd=="L1APARAMS") {
    int ilink=BaseMenu::readline_int("Which link? ",-1);    
    if (ilink>=0) {
      uint32_t reg1=pft->wb->wb_read(pflib::tgt_DAQ_Inbuffer,(ilink<<7)|1);
      int delay=BaseMenu::readline_int("L1A delay? ",(reg1>>8)&0xFF);
      int capture=BaseMenu::readline_int("L1A capture length? ",(reg1>>16)&0xFF);
      reg1=(reg1&0xFF)|((delay&0xFF)<<8)|((capture&0xFF)<<16);
      pft->wb->wb_write(pflib::tgt_DAQ_Inbuffer,(ilink<<7)|1,reg1);
    }
  }
  if (cmd=="DMA") {
#ifdef PFTOOL_ROGUE
    auto rwbi=dynamic_cast<pflib::rogue::RogueWishboneInterface*>(pft->wb);
    if (rwbi) {
      bool enabled;
      uint8_t samples_per_event, fpgaid_i;
      rwbi->daq_get_dma_setup(fpgaid_i,samples_per_event, enabled);
      enabled=BaseMenu::readline_bool("Enable DMA? ",enabled);
      rwbi->daq_dma_enable(enabled);
    } else {
      std::cout << "\nNot connected to chip with RogueWishboneInterface, cannot activate DMA.\n" << std::endl;
    }
#endif
  }
  if (cmd=="STANDARD") {
    daq_setup("FPGA",pft);
    pflib::Elinks& elinks=pft->hcal.elinks();
    for (int i=0; i<daq.nlinks(); i++) {
      if (elinks.isActive(i)) daq.setupLink(i,false,false,15,40);
      else daq.setupLink(i,true,true,15,40);
    }
  }
  if (cmd=="FPGA") {
    int fpgaid=BaseMenu::readline_int("FPGA id: ",daq.getFPGAid());
    daq.setIds(fpgaid);
#ifdef PFTOOL_ROGUE
    auto rwbi=dynamic_cast<pflib::rogue::RogueWishboneInterface*>(pft->wb);
    if (rwbi) {
      bool enabled;
      uint8_t samples_per_event, fpgaid_i;
      rwbi->daq_get_dma_setup(fpgaid_i,samples_per_event, enabled);
      fpgaid_i=(uint8_t(fpgaid));
      rwbi->daq_dma_setup(fpgaid_i,samples_per_event);
    }
#endif
  }
}





/**
 * Print data words from raw binary file and return them
 * @return vector of 32-bit data words in file
 */
std::vector<uint32_t> read_words_from_file() {
  std::vector<uint32_t> data;
  /// load from file
  std::string fname=BaseMenu::readline("Read from text file (line by line hex 32-bit words)");
  char buffer[512];
  FILE* f=fopen(fname.c_str(),"r");
  if (f==0) {
    printf("Unable to open '%s'\n",fname.c_str());
    return data;
  }
  while (!feof(f)) {
    buffer[0]=0;
    fgets(buffer,511,f);
    if (strlen(buffer)<8 || strchr(buffer,'#')!=0) continue;
    uint32_t val;
    val=strtoul(buffer,0,16);
    data.push_back(val);
    printf("%08x\n",val);
  }
  fclose(f);
  return data;
}

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
static void daq_debug( const std::string& cmd, PolarfireTarget* pft ) {
  if (cmd=="STATUS") {
    // get the general status
    daq("STATUS", pft); 
    uint32_t reg1,reg2;
    printf("-----Per-ROC Controls-----\n");
    reg1=pft->wb->wb_read(pflib::tgt_DAQ_Control,1);
    printf(" Disable ROC links: %s\n",(reg1&0x80000000u)?("TRUE"):("FALSE"));

    printf(" Link  F E RP WP \n");
    for (int ilink=0; ilink<PolarfireTarget::NLINKS; ilink++) {
      uint32_t reg=pft->wb->wb_read(pflib::tgt_DAQ_Inbuffer,(ilink<<7)|3);
      printf("   %2d  %d %d %2d %2d       %08x\n",ilink,(reg>>26)&1,(reg>>27)&1,(reg>>16)&0xf,(reg>>12)&0xf,reg);
    }
    printf("-----Event builder    -----\n");
    reg1=pft->wb->wb_read(pflib::tgt_DAQ_Control,6);
    printf(" EVB Debug word: %08x\n",reg1);
    reg1=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,1);
    reg2=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,4);
    printf(" Event buffer Debug word: %08x %08x\n",reg1,reg2);

    printf("-----Full event buffer-----\n");
    reg1=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,1);
    reg2=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,2);    printf(" Read Page: %d  Write Page : %d   Full: %d  Empty: %d   Evt Length on current page: %d\n",(reg1>>13)&0x1,(reg1>>12)&0x1,(reg1>>15)&0x1,(reg1>>14)&0x1,(reg1>>0)&0xFFF);
    printf(" Spy page : %d  Spy-as-source : %d  Length-of-spy-injected-event : %d\n",reg2&0x1,(reg2>>1)&0x1,(reg2>>16)&0xFFF);
  }
  if (cmd=="FULL_DEBUG") {
    uint32_t reg2=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,2);
    reg2=reg2^0x2;// invert
    pft->wb->wb_write(pflib::tgt_DAQ_Outbuffer,2,reg2);
  }
  if (cmd=="DISABLE_ROCLINKS") {
    uint32_t reg=pft->wb->wb_read(pflib::tgt_DAQ_Control,1);
    reg=reg^0x80000000u;// invert
    pft->wb->wb_write(pflib::tgt_DAQ_Control,1,reg);
  }
  if (cmd=="FULL_SEND") {
    uint32_t reg2=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,2);
    reg2=reg2|0x1000;// set the send bit
    pft->wb->wb_write(pflib::tgt_DAQ_Outbuffer,2,reg2);
  }
  if (cmd=="ROC_SEND") {
    uint32_t reg2=pft->wb->wb_read(pflib::tgt_DAQ_Control,1);
    reg2=reg2|0x40000000;// set the send bit
    pft->wb->wb_write(pflib::tgt_DAQ_Control,1,reg2);
  }
  if (cmd=="FULL_LOAD") {
    std::vector<uint32_t> data=read_words_from_file();

    // set the spy page to match the read page
    uint32_t reg1=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,1);
    uint32_t reg2=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,2);
    int rpage=(reg1>>12)&0x1;
    if (rpage) reg2=reg2|0x1;
    else reg2=reg2&0xFFFFFFFEu;
    pft->wb->wb_write(pflib::tgt_DAQ_Outbuffer,2,reg2);

    for (size_t i=0; i<data.size(); i++)
      pft->wb->wb_write(pflib::tgt_DAQ_Outbuffer,0x1000+i,data[i]);

    /// set the length
    reg2=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,2);
    reg2=reg2&0xFFFF;// remove the upper bits
    reg2=reg2|(data.size()<<16);
    pft->wb->wb_write(pflib::tgt_DAQ_Outbuffer,2,reg2);
  }
  if (cmd=="IBSPY") {
    static int input=0;
    input=BaseMenu::readline_int("Which input?",input);
    uint32_t reg=pft->wb->wb_read(pflib::tgt_DAQ_Inbuffer,(input<<7)|3);
    int rp=BaseMenu::readline_int("Read page?",(reg>>16)&0xF);
    reg=reg&0xFFFFFFF0u;
    reg=reg|(rp&0xF);
    pft->wb->wb_write(pflib::tgt_DAQ_Inbuffer,((input<<7)|3),reg);
    for (int i=0; i<40; i++) {
      uint32_t val=pft->wb->wb_read(pflib::tgt_DAQ_Inbuffer,(input<<7)|0x40|i);
      printf("%2d %08x\n",i,val);
    }
  }
  if (cmd=="EFSPY") {
    static int input=0;
    input=BaseMenu::readline_int("Which input?",input);
    pft->wb->wb_write(pflib::tgt_DAQ_LinkFmt,(input<<7)|3,0);
    uint32_t reg=pft->wb->wb_read(pflib::tgt_DAQ_LinkFmt,(input<<7)|4);
    printf("PTRs now: 0x%08x\n",reg);
    int rp=BaseMenu::readline_int("Read page?",reg&0xF);
    for (int i=0; i<40; i++) {
      pft->wb->wb_write(pflib::tgt_DAQ_LinkFmt,(input<<7)|3,0x400|(rp<<6)|i);
      uint32_t val=pft->wb->wb_read(pflib::tgt_DAQ_LinkFmt,(input<<7)|4);
      printf("%2d %08x\n",i,val);
    }
    pft->wb->wb_write(pflib::tgt_DAQ_LinkFmt,(input<<7)|3,0);
  }
  if (cmd=="SPY") {
    // set the spy page to match the read page
    uint32_t reg1=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,1);
    uint32_t reg2=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,2);
    int rpage=(reg1>>12)&0x1;
    if (rpage) reg2=reg2|0x1;
    else reg2=reg2&0xFFFFFFFEu;
    pft->wb->wb_write(pflib::tgt_DAQ_Outbuffer,2,reg2);

    for (size_t i=0; i<32; i++) {
      uint32_t val=pft->wb->wb_read(pflib::tgt_DAQ_Outbuffer,0x1000|i);
      printf("%04d %08x\n",int(i),val);
    }
  }

  if (cmd=="ROC_LOAD") {
    std::vector<uint32_t> data=read_words_from_file();
    if (int(data.size())!=PolarfireTarget::NLINKS*40) {
      printf("Expected %d words, got only %d\n",PolarfireTarget::NLINKS*40,int(data.size()));
      return;
    }
    for (int ilink=0; ilink<PolarfireTarget::NLINKS; ilink++) {
      uint32_t reg;
      // set the wishbone page to match the read page, and set the length
      reg=pft->wb->wb_read(pflib::tgt_DAQ_Inbuffer,(ilink<<7)+3);
      int rpage=(reg>>16)&0xF;
      reg=(reg&0xFFFFF000u)|rpage|(40<<4);
      pft->wb->wb_write(pflib::tgt_DAQ_Inbuffer,(ilink<<7)+3,reg);
      // load the bytes
      for (int i=0; i<40; i++)
        pft->wb->wb_write(pflib::tgt_DAQ_Inbuffer,(ilink<<7)|0x40|i,data[40*ilink+i]);
    }
  }
}

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
static void bias( const std::string& cmd, PolarfireTarget* pft ) {
  static int iboard=0;
  if (cmd=="STATUS") {
    iboard=BaseMenu::readline_int("Which board? ",iboard);
    pflib::Bias bias=pft->hcal.bias(iboard);
  }

  if (cmd=="INIT") {
    iboard=BaseMenu::readline_int("Which board? ",iboard);
    pflib::Bias bias=pft->hcal.bias(iboard);
    bias.initialize();
  }
  if (cmd=="SET") {
    iboard=BaseMenu::readline_int("Which board? ",iboard);
    static int led_sipm=0;
    led_sipm=BaseMenu::readline_int(" SiPM(0) or LED(1)? ",led_sipm);
    int ichan=BaseMenu::readline_int(" Which HDMI connector? ",-1);
    int dac=BaseMenu::readline_int(" What DAC value? ",0);
    if (ichan>=0) {
      pft->setBiasSetting(iboard,led_sipm==1,ichan,dac);
    }
    if (ichan==-1) {
      printf("\n Setting bias on all 16 connectors. \n");
      for(int k = 0; k < 16; k++){
        pft->setBiasSetting(iboard,led_sipm==1,k,dac);
      }
    }
  }
  if (cmd=="LOAD") {
    printf("\n --- This command expects a CSV file with four columns [0=SiPM/1=LED,board,hdmi#,value].\n");
    printf(" --- Line starting with # are ignored.\n");
    std::string fname=BaseMenu::readline("Filename: ");
    if (not pft->loadBiasSettings(fname)) {
      std::cerr << "\n\n  ERROR: Unable to access " << fname << std::endl;
    }
  }
}

/**
 * Menu construction for pftool
 *
 * Currently, we must manually list all of the menus and their
 * commands and sub-menus somewhere. This function is just helpful for
 * keeping the listing isolated.
 *
 * @note Again, this listing is _manual_. It is very easy to implement
 * a new command within one of the menu functions but forget to repeat
 * its listing here.
 *
 * @param[in] pft target that has been initialized
 */
static void RunMenu( PolarfireTarget* pft_ ) {
  using pfMenu = Menu<PolarfireTarget>;

  pfMenu menu_wishbone({
    pfMenu::Line("RESET", "Enable/disable (toggle)",  &wb ),
    pfMenu::Line("READ", "Read from an address",  &wb ),
    pfMenu::Line("BLOCKREAD", "Read several words starting at an address",  &wb ),
    pfMenu::Line("WRITE", "Write to an address",  &wb ),
    pfMenu::Line("STATUS", "Wishbone errors counters",  &wb ),
    pfMenu::Line("QUIT","Back to top menu")
    });

  pfMenu menu_i2c({
    pfMenu::Line("BUS","Pick the I2C bus to use", &i2c ),
    pfMenu::Line("READ", "Read from an address",  &i2c ),
    pfMenu::Line("WRITE", "Write to an address",  &i2c ),
    pfMenu::Line("MULTIREAD", "Read from an address",  &i2c ),
    pfMenu::Line("MULTIWRITE", "Write to an address",  &i2c ),
    pfMenu::Line("QUIT","Back to top menu")
    });

  pfMenu menu_link({
      /*
    pfMenu::Line("STATUS","Dump link status", &link ),
    pfMenu::Line("CONFIG","Setup link", &link ),
    pfMenu::Line("SPY", "Spy on the uplink",  &link ),
    */
    pfMenu::Line("QUIT","Back to top menu")
    });

  pfMenu menu_elinks({
    pfMenu::Line("RELINK","Follow standard procedure to establish links", &elinks),
    pfMenu::Line("HARD_RESET","Hard reset of the PLL", &elinks),
    pfMenu::Line("STATUS", "Elink status summary",  &elinks ),
    pfMenu::Line("SPY", "Spy on an elink",  &elinks ),
    pfMenu::Line("HEADER_CHECK", "Do a pedestal run and tally good/bad headers, only non-DMA", &elinks),
    pfMenu::Line("BITSLIP", "Set the bitslip for a link or turn on auto", &elinks),
    pfMenu::Line("SCAN", "Scan on an elink",  &elinks ),
    pfMenu::Line("DELAY", "Set the delay on an elink", &elinks),
    pfMenu::Line("BIGSPY", "Take a spy of a specific channel at 32-bits", &elinks),
    pfMenu::Line("QUIT","Back to top menu")
  });

  pfMenu menu_roc({
     pfMenu::Line("HARDRESET","Hard reset to all rocs", &roc),
     pfMenu::Line("SOFTRESET","Soft reset to all rocs", &roc),
     pfMenu::Line("RESYNCLOAD","ResyncLoad to specified roc to help maintain link stability", &roc),
     pfMenu::Line("IROC","Change the active ROC number", &roc ),
     pfMenu::Line("CHAN","Dump link status", &roc ),
     pfMenu::Line("PAGE","Dump a page", &roc ),
     pfMenu::Line("PARAM_NAMES", "Print a list of parameters on a page", &roc),
     pfMenu::Line("POKE_REG","Change a single register value", &roc ),
     pfMenu::Line("POKE_PARAM","Change a single parameter value", &roc ),
     pfMenu::Line("POKE","Alias for POKE_PARAM", &roc ),
     pfMenu::Line("LOAD_REG","Load register values onto the chip from a CSV file", &roc ),
     pfMenu::Line("LOAD_PARAM","Load parameter values onto the chip from a YAML file", &roc ),
     pfMenu::Line("LOAD","Alias for LOAD_PARAM", &roc ),
     pfMenu::Line("DUMP","Dump hgcroc settings to a file", &roc ),
     pfMenu::Line("DEFAULT_PARAMS", "Load default YAML files", &roc),
     pfMenu::Line("QUIT","Back to top menu")
    }, roc_render);

  pfMenu menu_bias({
  //  pfMenu::Line("STATUS","Read the bias line settings", &bias ),
    pfMenu::Line("INIT","Initialize a board", &bias ),
    pfMenu::Line("SET","Set a specific bias line setting", &bias ),
    pfMenu::Line("LOAD","Load bias values from file", &bias ),
    pfMenu::Line("QUIT","Back to top menu"),
  });

  pfMenu menu_fc({
    pfMenu::Line("STATUS","Check status and counters", &fc ),
    pfMenu::Line("SW_L1A","Send a software L1A", &fc ),
    pfMenu::Line("LINK_RESET","Send a link reset", &fc ),
    pfMenu::Line("BUFFER_CLEAR","Send a buffer clear", &fc ),
    pfMenu::Line("RUN_CLEAR","Send a run clear", &fc ),
    pfMenu::Line("COUNTER_RESET","Reset counters", &fc ),
    pfMenu::Line("FC_RESET","Reset the fast control", &fc ),
    pfMenu::Line("VETO_SETUP","Setup the L1 Vetos", &fc ),
    pfMenu::Line("MULTISAMPLE","Setup multisample readout", &fc ),
    pfMenu::Line("CALIB","Setup calibration pulse", &fc ),
    pfMenu::Line("ENABLES","Enable various sources of signal", &fc ),
    pfMenu::Line("QUIT","Back to top menu")
  });

  pfMenu menu_daq_debug({
    pfMenu::Line("STATUS","Provide the status", &daq_debug ),
    pfMenu::Line("FULL_DEBUG", "Toggle debug mode for full-event buffer",  &daq_debug ),
    pfMenu::Line("DISABLE_ROCLINKS", "Disable ROC links to drive only from SW",  &daq_debug ),
    pfMenu::Line("READ", "Read an event", &daq),
    pfMenu::Line("ROC_LOAD", "Load a practice ROC events from a file",  &daq_debug ),
    pfMenu::Line("ROC_SEND", "Generate a SW L1A to send the ROC buffers to the builder",  &daq_debug ),
    pfMenu::Line("FULL_LOAD", "Load a practice full event from a file",  &daq_debug ),
    pfMenu::Line("FULL_SEND", "Send the buffer to the off-detector electronics",  &daq_debug ),
    pfMenu::Line("SPY", "Spy on the front-end buffer",  &daq_debug ),
    pfMenu::Line("IBSPY","Spy on an input buffer",  &daq_debug ),
    pfMenu::Line("EFSPY","Spy on an event formatter buffer",  &daq_debug ),
    pfMenu::Line("QUIT","Back to top menu")
  });

  pfMenu menu_daq_setup({
    pfMenu::Line("STATUS", "Status of the DAQ", &daq_setup),
    pfMenu::Line("ENABLE", "Toggle enable status", &daq_setup),
    pfMenu::Line("ZS", "Toggle ZS status", &daq_setup),
    pfMenu::Line("L1APARAMS", "Setup parameters for L1A capture", &daq_setup),
    pfMenu::Line("FPGA", "Set FPGA id", &daq_setup),
    pfMenu::Line("STANDARD","Do the standard setup for HCAL", &daq_setup),
    pfMenu::Line("MULTISAMPLE","Setup multisample readout", &fc ),
#ifdef PFTOOL_ROGUE
    pfMenu::Line("DMA", "Enable/disable DMA readout (only available with rogue)", &daq_setup),
#endif
    pfMenu::Line("QUIT","Back to DAQ menu")
  });

  pfMenu menu_daq({
    pfMenu::Line("DEBUG", "Debugging menu",  &menu_daq_debug ),
    pfMenu::Line("STATUS", "Status of the DAQ", &daq),
    pfMenu::Line("SETUP", "Setup the DAQ", &menu_daq_setup),
    pfMenu::Line("RESET", "Reset the DAQ", &daq),
    pfMenu::Line("HARD_RESET", "Reset the DAQ, including all parameters", &daq),
    pfMenu::Line("PEDESTAL","Take a simple random pedestal run", &daq),
    pfMenu::Line("CHARGE","Take a charge-injection run", &daq),
    pfMenu::Line("EXTERNAL","Take an externally-triggered run", &daq),
    //    pfMenu::Line("SCAN","Take many charge or pedestal runs while changing a single parameter", &daq),
    pfMenu::Line("QUIT","Back to top menu")
  });

  pfMenu menu_expert({
    pfMenu::Line("OLINK","Optical link functions", &menu_link),
    pfMenu::Line("WB","Raw wishbone interactions", &menu_wishbone ),
    pfMenu::Line("I2C","Access the I2C Core", &menu_i2c ),
    pfMenu::Line("QUIT","Back to top menu")
  });

  pfMenu menu_tasks({
    pfMenu::Line("RESET_POWERUP", "Execute FC,ELINKS,DAQ reset after power up", &tasks),
    pfMenu::Line("SCANCHARGE","Charge scan over all active channels", &tasks),
    pfMenu::Line("DELAYSCAN","Charge injection delay scan", &tasks ),
    pfMenu::Line("QUIT","Back to top menu")
  });

  pfMenu menu_utop({
    pfMenu::Line("STATUS","Status summary", &status),
    pfMenu::Line("TASKS","Various high-level tasks like scans", &menu_tasks ),
    pfMenu::Line("FAST_CONTROL","Fast Control", &menu_fc ),
    pfMenu::Line("ROC","ROC Configuration", &menu_roc ),
    pfMenu::Line("BIAS","BIAS voltage setting", &menu_bias ),
    pfMenu::Line("ELINKS","Manage the elinks", &menu_elinks ),
    pfMenu::Line("DAQ","DAQ", &menu_daq ),
    pfMenu::Line("EXPERT","Expert functions", &menu_expert ),
    pfMenu::Line("EXIT","Exit this tool")
  });

  menu_utop.steer( pft_ ) ;
}

/**
 * Check if a file exists by attempting to open it for reading
 * @param[in] fname name of file to check
 * @return true if file can be opened, false otherwise
 */
bool file_exists(const std::string& fname) {
  FILE* f=fopen(fname.c_str(),"r");
  if (f==0) return false;
  fclose(f);
  return true;
}

/**
 * List the options that could be provided within the RC file
 *
 * @param[in] rcfile RC file prepare
 */
void prepareOpts(Rcfile& rcfile) {
  rcfile.declareVBool("roclinks",
      "Vector Bool[8] indicating which roc links are active");
  rcfile.declareString("ipbus_map_path",
      "Full path to directory containgin IP-bus mapping. Only required for uHal comm.");
  rcfile.declareString("default_hostname",
      "Hostname of polarfire to connect to if none are given on the command line");
  rcfile.declareVString("roc_configs", "Vector String[4] pointing to default .yaml configs for each roc");
  rcfile.declareString("runnumber_file",
                       "Full path to a file which contains the last run number");
}

/**
 * This is the main executable for the pftool.
 *
 * The options are prepared first so their help information
 * is available for the usage information.
 *
 * If '-h' or '--help' is the only parameter provided,
 * the usage information is printed and we exit; otherwise,
 * we continue.
 *
 * The RC files  are read from ${PFTOOLRC}, pftoolrc, and ~/.pftoolrc
 * with priority in that order.
 * Then the command line parameters are parsed; if not hostname(s) is(are)
 * provided on the command line, then the 'default_hostname' RC file option
 * is necessary.
 *
 * Initialization scripts (provided with `-s`) are parsed such that white space
 * and lines which start with `#` are ignored. The keystrokes in those "sripts"
 * are run upon launching of the menu and then the user gets control of the tool
 * back after the script is done.
 */
int main(int argc, char* argv[]) {
  Rcfile options;
  prepareOpts(options);

  // print help
  if (argc == 2 and (!strcmp(argv[1],"-h") or !strcmp(argv[1],"--help"))) {
#ifdef PFTOOL_ROGUE
#ifdef PFTOOL_UHAL
    printf("Usage: pftool [hostname] [-u] [-r] [-s script]\n");
    printf("  Supporting both UHAL and ROGUE\n");
    printf("   -u uHAL connection\n");
    printf("   -r rogue connection\n");
#else
    printf("Usage: pftool [hostname] [-s script]\n");
    printf("  Supporting only ROGUE\n");
#endif
#else
#ifdef PFTOOL_UHAL
    printf("Usage: pftool [hostname] [-s script]\n");
    printf("  Supporting only UHAL\n");
#else
    printf("Usage: pftool [hostname] [-s script]\n");
    printf("  Supporting NO TRANSPORTS\n");
#endif
#endif
    printf("Reading RC files from ${PFTOOLRC}, ${CWD}/pftoolrc, ${HOME}/.pftoolrc with priority in this order\n");
    options.help();

    printf("\n");
    return 1;
  }

  /*****************************************************************************
   * Load RC File
   ****************************************************************************/
  std::string home=getenv("HOME");
  if (getenv("PFTOOLRC")) {
    if (file_exists(getenv("PFTOOLRC"))) {
      options.load(getenv("PFTOOLRC"));
    } else {
      std::cerr << "WARNING: PFTOOLRC=" << getenv("PFTOOLRC")
        << " is not loaded because it doesn't exist." << std::endl;
    }
  }
  if (file_exists("pftoolrc"))
    options.load("pftoolrc");
  if (file_exists(home+"/.pftoolrc"))
    options.load(home+"/.pftoolrc");
 

  /*****************************************************************************
   * Parse Command Line Parameters
   ****************************************************************************/
  bool isuhal=false;
  bool isrogue=false;
  // rogue is default if it is available
#ifdef PFTOOL_ROGUE
  isrogue=true; // default
#else
  isuhal=true; // default
#endif
  std::vector<std::string> hostnames;
  for (int i = 1 ; i < argc ; i++) {
    std::string arg(argv[i]);
    if (arg=="-s") {
      if (i+1 == argc or argv[i+1][0] == '-') {
        std::cerr << "Argument " << arg << " requires a file after it." << std::endl;
        return 2;
      }
      i++;
      std::fstream sFile(argv[i]);
      if (!sFile.is_open()) {
        std::cerr << "Unable to open script file " << argv[i] << std::endl;
        return 2;
      }

      std::string line;
      while (getline(sFile, line)) {
        // erase whitespace at beginning of line
        while (!line.empty() && isspace(line[0])) line.erase(line.begin());
        // skip empty lines or ones whose first character is #
        if ( !line.empty() && line[0] == '#' ) continue ;
        // add to command queue
        BaseMenu::add_to_command_queue(line);
      }
      sFile.close() ;
    }
#ifdef PFTOOL_UHAL
#ifdef PFTOOL_ROGUE
    else if (arg=="-u") {
      isuhal  = true;
      isrogue = false;
    }
    else if (arg=="-r") {
      isrogue = true;
      isuhal  = false;
    }
#endif
#endif
    else {
      // positional argument -> hostname
      hostnames.push_back( arg ) ;
    }
  }

  if (hostnames.size() == 0) {
    std::string hn = options.contents().getString("default_hostname");
    if (hn.empty()) {
      std::cerr << "No hostnames to connect to provided on the command line or in RC file" << std::endl;
      return 3;
    } else {
      hostnames.push_back(hn);
    }
  }

  /*****************************************************************************
   * Run tool
   ****************************************************************************/
  try {
    int i_host{-1};
    bool continue_running = true; // used if multiple hosts
    do {
      if (hostnames.size() > 1) {
        while (true) {
          std::cout << "ID - Hostname" << std::endl;
          for (std::size_t k{0}; k < hostnames.size(); k++) {
            std::cout << std::setw(2) << k << " - " << hostnames.at(k) << std::endl;
          }
          i_host = BaseMenu::readline_int(" ID of Polarfire Hostname (-1 to exit) : ", i_host);
          if (i_host == -1 or (i_host >= 0 and i_host < hostnames.size())) {
            // valid choice, let's leave
            break;
          } else {
            std::cerr << "\n " << i_host << " is not a valid choice." << std::endl;
          }
        }
        // if user chooses to leave menu
        if (i_host == -1) break;
      } else {
        i_host = 0;
      }

      // initialize connect with Polarfire
      std::unique_ptr<PolarfireTarget> p_pft;
      try {
#ifdef PFTOOL_ROGUE
        if (isrogue) {
          // the PolarfireTarget wraps the passed pointers in STL smart pointers so the memory will be handled
          p_pft=std::make_unique<PolarfireTarget>(new pflib::rogue::RogueWishboneInterface(hostnames.at(i_host),5970));
        }
#endif
#ifdef PFTOOL_UHAL
        if (isuhal) {
          // the PolarfireTarget wraps the passed pointers in STL smart pointers so the memory will be handled
          p_pft=std::make_unique<PolarfireTarget>(new pflib::uhal::uhalWishboneInterface(hostnames.at(i_host),
                options.contents().getString("ipbus_map_path")));
        }
#endif
      } catch (const pflib::Exception& e) {
        std::cerr << "pflib Init Error [" << e.name() << "] : " << e.message() << std::endl;
        return 3;
      }

      if (options.contents().has_key("runnumber_file"))
        last_run_file=options.contents().getString("runnumber_file");

      if (p_pft) {
      	// prepare the links
      	if (options.contents().is_vector("roclinks")) {
      	  std::vector<bool> actives=options.contents().getVBool("roclinks");
      	  for (int ilink=0;
              ilink<p_pft->hcal.elinks().nlinks() and ilink<int(actives.size());
              ilink++) p_pft->hcal.elinks().markActive(ilink,actives[ilink]);
      	}

        if (options.contents().is_vector("roc_configs")) {
          p_pft->findDefaultROCParameters(options.contents().getVString("roc_configs"));
        }

        status(p_pft.get());
        RunMenu(p_pft.get());
      } else {
        std::cerr << "No Polarfire Target available to connect with. Not sure how we got here." << std::endl;
        return 126;
      }

      if (hostnames.size() > 1)  {
        // menu for that target has been exited, check if user wants to choose another host
        continue_running = BaseMenu::readline_bool(" Choose a new card/host to connect to ? ", true);
      } else {
        // no other hosts, leave
        continue_running = false;
      }
    } while(continue_running);
  } catch (const std::exception& e) {
    std::cerr << " Unrecognized Exception : " << e.what() << std::endl;
    return 127;
  }
  return 0;
}
