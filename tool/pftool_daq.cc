#include "pftool_daq.h"



std::string last_run_file=".last_run_file";
std::string start_dma_cmd="";
std::string stop_dma_cmd="";
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
    std::string fname_def_format = "pedestal_%Y%m%d_%H%M%S.raw";
    if (cmd=="CHARGE") fname_def_format = "charge_%Y%m%d_%H%M%S.raw";

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    char fname_def[64];
    strftime(fname_def, sizeof(fname_def), fname_def_format.c_str(), tm);

    int run=BaseMenu::readline_int("Run number? ",run);
    int nevents=BaseMenu::readline_int("How many events? ", 100);
    static int rate=100;
    rate=BaseMenu::readline_int("Readout rate? (Hz) ",rate);
    std::string fname=BaseMenu::readline("Filename :  ", fname_def);

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
