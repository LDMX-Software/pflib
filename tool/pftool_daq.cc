#include "pftool_daq.h"

using pflib::PolarfireTarget;

std::string last_run_file=".last_run_file";
std::string start_dma_cmd="";
std::string stop_dma_cmd="";


std::string make_default_daq_run_filename(const std::string& cmd) {
  std::string fname_def_format = "pedestal_%Y%m%d_%H%M%S.raw";
  if (cmd == "CHARGE") {
    fname_def_format = "charge_%Y%m%d_%H%M%S.raw";
  }
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  char fname_def[64];
  strftime(fname_def, sizeof(fname_def), fname_def_format.c_str(), tm);
  return fname_def;
}


void daq_run(
  PolarfireTarget* pft,
  const std::string& cmd // PEDESTAL, CHARGE, or no trigger
  , int run // not used in this implementation of daq
  , int nevents // number of events to collect
  , int rate // not used in this implementation of daq
  , const std::string& fname // file to write to (appended)
) {
  std::unique_ptr<FILE, int (*)(FILE*)> fp{fopen(fname.c_str(),"a"),&fclose};
  timeval tv0, tvi;

  gettimeofday(&tv0,0);

  for (int ievt=0; ievt<nevents; ievt++) {      // normally, some other controller would send the L1A
    //  we are sending it so we get data during no signal
    if (cmd=="PEDESTAL")
      pft->backend->fc_sendL1A();
    if (cmd=="CHARGE")
      pft->backend->fc_calibpulse();

    gettimeofday(&tvi,0);
    double runsec=(tvi.tv_sec-tv0.tv_sec)+(tvi.tv_usec-tvi.tv_usec)/1e6;
    //      double ratenow=(ievt+1)/runsec;
    double targettime=(ievt+1.0)/rate; // what I'd like the rate to be
    int usec_ahead=int((targettime-runsec)*1e6);
    //printf("Sleeping %f %f %d\n",runsec,targettime,usec_ahead);
    if (usec_ahead>100) { // if we are running fast...
      usleep(usec_ahead);
      //        printf("Sleeping %d\n",usec_ahead);
    }

    std::vector<uint32_t> event = pft->daqReadEvent();
    pft->backend->fc_advance_l1_fifo();
    fwrite(&(event[0]),sizeof(uint32_t),event.size(),fp.get());
  }
}

std::vector<uint32_t> read_words_from_file()
{
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

void daq( const std::string& cmd, PolarfireTarget* pft )
{
  pflib::DAQ& daq=pft->hcal.daq();

  // default is non-DMA readout
  bool dma_enabled=false;
  auto daq_run = [&](const std::string& cmd // PEDESTAL, CHARGE, or no trigger
      , int run // not used in this implementation of daq
      , int nevents // number of events to collect
      , int rate // not used in this implementation of daq
      , const std::string& fname // file to write to (appended)
  ) {
    std::unique_ptr<FILE, int (*)(FILE*)> fp{fopen(fname.c_str(),"a"),&fclose};
    timeval tv0, tvi;

    gettimeofday(&tv0,0);

    for (int ievt=0; ievt<nevents; ievt++) {
      // normally, some other controller would send the L1A
      //  we are sending it so we get data during no signal
      if (cmd=="PEDESTAL")
        pft->backend->fc_sendL1A();
      if (cmd=="CHARGE")
        pft->backend->fc_calibpulse();

      gettimeofday(&tvi,0);
      double runsec=(tvi.tv_sec-tv0.tv_sec)+(tvi.tv_usec-tvi.tv_usec)/1e6;
      //      double ratenow=(ievt+1)/runsec;
      double targettime=(ievt+1.0)/rate; // what I'd like the rate to be
      int usec_ahead=int((targettime-runsec)*1e6);
      //printf("Sleeping %f %f %d\n",runsec,targettime,usec_ahead);
      if (usec_ahead>100) { // if we are running fast...
        usleep(usec_ahead);
        //        printf("Sleeping %d\n",usec_ahead);
      }

      std::vector<uint32_t> event = pft->daqReadEvent();
      pft->backend->fc_advance_l1_fifo();
      fwrite(&(event[0]),sizeof(uint32_t),event.size(),fp.get());
    }
  };

#ifdef PFTOOL_ROGUE
  auto rwbi=dynamic_cast<pflib::rogue::RogueWishboneInterface*>(pft->wb);
  uint8_t fpgaid_i;
  if (rwbi) {
    uint8_t samples_per_event;
    rwbi->daq_get_dma_setup(fpgaid_i,samples_per_event, dma_enabled);
  }
#endif

  if (cmd=="STATUS") {
    pft->daqStatus(std::cout);
#ifdef PFTOOL_ROGUE
    auto rwbi=dynamic_cast<pflib::rogue::RogueWishboneInterface*>(pft->wb);
    if (rwbi) {
      printf("DMA : %s Status=%08x\n",(dma_enabled)?("ENABLED"):("DISABLED"),rwbi->daq_dma_status());
    }
#endif
  }
  if (cmd=="RESET") {
    pft->daqSoftReset();
  }
  if (cmd=="HARD_RESET") {
    pft->daqHardReset();
  }
  if (cmd=="READ") {
    std::vector<uint32_t> buffer = pft->daqReadDirect();
    for (size_t i=0; i<buffer.size() && i<32; i++) {
      printf("%04d %08x\n",int(i),buffer[i]);
    }
    bool save_to_disk=BaseMenu::readline_bool("Save to disk?  ",false);
    if (save_to_disk) {
      std::string fname=BaseMenu::readline("Filename :  ");
      FILE* f=fopen(fname.c_str(),"w");
      fwrite(&(buffer[0]),sizeof(uint32_t),buffer.size(),f);
      fclose(f);
    }
  }
  if (cmd=="EXTERNAL") {
    int run=0;

    FILE* run_no_file=fopen(last_run_file.c_str(),"r");
    if (run_no_file) {
      fscanf(run_no_file,"%d",&run);
      fclose(run_no_file);
      run=run+1;
    }

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    struct tm *gmtm = gmtime(&t);

    run=BaseMenu::readline_int("Run number? ",run);

    char fname_def_format[1024];
    sprintf(fname_def_format,"fpga%d_run%06d_%%Y%%m%%d_%%H%%M%%S.raw",fpgaid_i,run);
    char fname_def[1024];
    strftime(fname_def, sizeof(fname_def), fname_def_format, tm);

    std::string fname=BaseMenu::readline("Filename :  ", fname_def);

    run_no_file=fopen(last_run_file.c_str(),"w");
    if (run_no_file) {
      fprintf(run_no_file,"%d\n",run);
      fclose(run_no_file);
    }

    FILE* f=0;
    if (!dma_enabled) f=fopen(fname.c_str(),"w");

    int event_target=BaseMenu::readline_int("Target events? ",1000);
    pft->backend->daq_setup_event_tag(run,gmtm->tm_mday,gmtm->tm_mon+1,gmtm->tm_hour,gmtm->tm_min);

    // reset various counters
    pft->prepareNewRun();

    // start DMA, if that's what we're doing...
    if (dma_enabled) rwbi->daq_dma_dest(fname);

    // enable external triggers
    pft->backend->fc_enables(true,true,false);

    int ievent=0, wasievent=0;
    while (ievent<event_target) {

      if (dma_enabled) {
        int spill,occ,occ_max,vetoed;
        pft->backend->fc_read_counters(spill,occ,occ_max,ievent,vetoed);
        if (ievent>wasievent) {
          printf("...Now read %d events\n",ievent);
          wasievent=ievent;
        } else {
          sleep(1);
        }
      } else {
        bool full, empty;
        int samples, esize;
        do {
          pft->backend->daq_status(full, empty, samples, esize);
        } while (empty);
        printf("%d: %d samples waiting for readout...\n",ievent,samples);

        if (f) {
          std::vector<uint32_t> event = pft->daqReadEvent();
          fwrite(&(event[0]),sizeof(uint32_t),event.size(),f);
        }
        pft->backend->fc_advance_l1_fifo();

        ievent++;
      }
    }

    // disable external triggers
    pft->backend->fc_enables(false,true,false);

    if (f) fclose(f);

    if (dma_enabled) rwbi->daq_dma_close();

  }
  if (cmd=="PEDESTAL" || cmd=="CHARGE") {

    std::string fname = make_default_daq_run_filename(cmd);
    int run=BaseMenu::readline_int("Run number? ",run);
    int nevents=BaseMenu::readline_int("How many events? ", 100);
    static int rate=100;
    rate=BaseMenu::readline_int("Readout rate? (Hz) ",rate);
    fname=BaseMenu::readline("Filename :  ", fname);

    pft->prepareNewRun();

#ifdef PFTOOL_ROGUE
    if (dma_enabled) {
      rwbi->daq_dma_dest(fname);
      rwbi->daq_dma_run(cmd,run,nevents,rate);
      rwbi->daq_dma_close();
    } else
#endif
    {
      daq_run(cmd,run,nevents,rate,fname);
    }
  }
  /** Deprecated by new TASK menu */
  if (cmd=="SCAN"){
    std::string pagename=BaseMenu::readline("Sub-block (aka Page) name :  ");
    std::string valuename=BaseMenu::readline("Value (aka Parameter) name :  ");
    int iroc=BaseMenu::readline_int("Which ROC :  ");
    int minvalue=BaseMenu::readline_int("Minimum value :  ");
    int maxvalue=BaseMenu::readline_int("Maximum value :  ");
    int step=BaseMenu::readline_int("Step :  ");
    int nevents=BaseMenu::readline_int("Events per step :  ", 10);
    int run=BaseMenu::readline_int("Run number? ",run);
    static int rate=100;
    rate=BaseMenu::readline_int("Readout rate? (Hz) ",rate);
    std::string fname=BaseMenu::readline("Filename :  ");
    bool charge = BaseMenu::readline_bool("Do a charge injection for each event rather than simple L1A?",false);
    std::string trigtype = "PEDESTAL";
    if (charge) trigtype = "CHARGE";

    pft->prepareNewRun();

    for(int value = minvalue; value <= maxvalue; value += step){
      pft->hcal.roc(iroc).applyParameter(pagename, valuename, value);
#ifdef PFTOOL_ROGUE
      if (dma_enabled) {
        rwbi->daq_dma_dest(fname);
        rwbi->daq_dma_run(trigtype,run,nevents,rate);
        rwbi->daq_dma_close();
      } else
#endif
      {
        daq_run(trigtype,run,nevents,rate,fname);
      }
    }
  }
}
void daq_debug( const std::string& cmd, pflib::PolarfireTarget* pft )
{
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

void daq_setup( const std::string& cmd, pflib::PolarfireTarget* pft )
{
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
    setup_dma(pft);
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

void setup_dma(PolarfireTarget* pft, bool force_to)
{
#ifdef PFTOOL_ROGUE
  auto rwbi = dynamic_cast<pflib::rogue::RogueWishboneInterface *>(pft->wb);
  if (rwbi) {
    rwbi->daq_dma_enable(force_to);
  } else {
    std::cout << "\nNot connected to chip with RogueWishboneInterface, cannot "
                 "activate DMA.\n"
              << std::endl;
  }
#endif
}
void setup_dma(PolarfireTarget* pft)
{
#ifdef PFTOOL_ROGUE
  auto rwbi = dynamic_cast<pflib::rogue::RogueWishboneInterface *>(pft->wb);
  if (rwbi) {
    bool enabled;
    uint8_t samples_per_event, fpgaid_i;
    rwbi->daq_get_dma_setup(fpgaid_i, samples_per_event, enabled);
    std::cout << "DMA Status before: " << enabled
              << " with "
              << samples_per_event
              << " samples per event\n";
    enabled = BaseMenu::readline_bool("Enable DMA? ", enabled);
    rwbi->daq_dma_enable(enabled);
    rwbi->daq_get_dma_setup(fpgaid_i, samples_per_event, enabled);
    std::cout << "DMA Status after: " << enabled
              << " with "
              << samples_per_event
              << " samples per event\n";
  } else {
    std::cout << "\nNot connected to chip with RogueWishboneInterface, cannot "
                 "activate DMA.\n"
              << std::endl;
  }
#endif
}
