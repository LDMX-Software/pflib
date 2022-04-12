#include "pftool_tasks.h"

void tasks( const std::string& cmd, pflib::PolarfireTarget* pft )
{
  pflib::DAQ& daq=pft->hcal.daq();

  static int low_value=10;
  static int high_value=500;
  static int steps=10;
  static int events_per_step=100;
  std::string pagetemplate;
  std::string valuename;
  std::string modeinfo;

  int nsamples=1;
  {
    bool multi;
    int nextra;
    pft->hcal.fc().getMultisampleSetup(multi,nextra);
    if (multi) nsamples=nextra+1;
  }

  if (cmd == "BEAMPREP") {
    beamprep(pft);
    return;
  }
  if (cmd=="RESETPOWERUP") {
    pft->hcal.fc().resetTransmitter();
    pft->hcal.resyncLoadROC();
    pft->daqHardReset();
    pflib::Elinks& elinks=pft->hcal.elinks();
    elinks.resetHard();
  }
  
  if (cmd=="SCANCHARGE") {
    valuename="CALIB_DAC";
    pagetemplate="REFERENCE_VOLTAGE_%d";
    printf("CALIB_DAC valid range is 0...4095\n");
    low_value=BaseMenu::readline_int("Smallest value of CALIB_DAC?",low_value);
    high_value=BaseMenu::readline_int("Largest value of CALIB_DAC?",high_value);
    bool is_high_range=BaseMenu::readline_bool("Use HighRange? ",false);
    if (is_high_range) modeinfo="HIGHRANGE";
    else modeinfo="LOWRANGE";
  }
  if (cmd == "SCANTOAVREF") {
    low_value = 0;
    high_value = 1000;
    steps = 50;
    valuename = "TOA_VREF";
    pagetemplate = "REFERENCE_VOLTAGE_%d";
    
  }
  /// common stuff for all scans
  if (cmd=="SCANCHARGE") {
    int len{};
    int offset{};
    pft->backend->fc_get_setup_calib(len,offset);
    static const int NUM_ELINK_CHAN=36;

    steps=BaseMenu::readline_int("Number of steps?",steps);
    events_per_step=BaseMenu::readline_int("Events per step?",events_per_step);

    time_t t=time(NULL);
    struct tm *tm = localtime(&t);
    char fname_def_format[1024];
    sprintf(fname_def_format,"scan_%s_coff%d_%%Y%%m%%d_%%H%%M%%S.csv",valuename.c_str(), offset);
    char fname_def[1024];
    strftime(fname_def, sizeof(fname_def), fname_def_format, tm); 
    
    std::string fname=BaseMenu::readline("Filename :  ", fname_def);
    std::ofstream csv_out=std::ofstream(fname);

    // csv header
    csv_out <<valuename << ",ILINK,CHAN,EVENT";
    for (int i=0; i<nsamples; i++) csv_out << ",ADC" << i;
    for (int i=0; i<nsamples; i++) csv_out << ",TOT" << i;
    for (int i=0; i<nsamples; i++) csv_out << ",TOA" << i;
    csv_out<<std::endl;

    printf(" Clearing charge injection on all channels (ground-state)...\n");
    
    /// first, disable charge injection on all channels
    for (int ilink=0; ilink<pft->hcal.elinks().nlinks(); ilink++) {
      if (!pft->hcal.elinks().isActive(ilink)) continue;

      int iroc=ilink/2;        
      for (int ichan=0; ichan<NUM_ELINK_CHAN; ichan++) {
        char pagename[32];
        snprintf(pagename,32,"CHANNEL_%d",(ilink%2)*(NUM_ELINK_CHAN)+ichan);
        // set the value
        pft->hcal.roc(iroc).applyParameter(pagename, "LOWRANGE", 0);
        pft->hcal.roc(iroc).applyParameter(pagename, "HIGHRANGE", 0);
      }
    }

    printf(" Enabling IntCTest...\n");
    
    for (int ilink=0; ilink<pft->hcal.elinks().nlinks(); ilink++) {
      if (!pft->hcal.elinks().isActive(ilink)) continue;

      int iroc=ilink/2;        
      char pagename[32];
      snprintf(pagename,32,"REFERENCE_VOLTAGE_%d",(ilink%2));
      // set the value
      pft->hcal.roc(iroc).applyParameter(pagename, "INTCTEST", 1);
      if (cmd=="SCANCHARGE") {
        snprintf(pagename,32,"DIGITAL_HALF_%d",(ilink%2));
        // set the value
        pft->hcal.roc(iroc).applyParameter(pagename, "L1OFFSET", 8);
      }
      if (cmd=="SCANTOAVREF") {
	// Choose a fixed CALIB DAC for charge injection
	snprintf(pagename,32,"REFERENCE_VOLTAGE_%d",(ilink%2));
	pft->hcal.roc(iroc).applyParameter(pagename, "CALIB_DAC", 400);
	// Choose a fixed TOT_VREF? based on configv0
	pft->hcal.roc(iroc).applyParameter(pagename, "TOT_VREF", 475);
      }
    }
    
    ////////////////////////////////////////////////////////////
    
    for (int step=0; step<steps; step++) {

      ////////////////////////////////////////////////////////////
      /// set values
      int value=low_value+step*(high_value-low_value)/std::max(1,steps-1);

      printf(" Scan %s=%d...\n",valuename.c_str(),value);
      
      for (int ilink=0; ilink<pft->hcal.elinks().nlinks(); ilink++) {
        if (!pft->hcal.elinks().isActive(ilink)) continue;

        int iroc=ilink/2;
        char pagename[32];
        snprintf(pagename,32,pagetemplate.c_str(),ilink%2);
        // set the value
        pft->hcal.roc(iroc).applyParameter(pagename, valuename, value);
      }
      ////////////////////////////////////////////////////////////

      pft->prepareNewRun();
      
      for (int ichan=0; ichan<NUM_ELINK_CHAN; ichan++) {

        ////////////////////////////////////////////////////////////
        /// Enable charge injection channel by channel -- per elink
        for (int ilink=0; ilink<pft->hcal.elinks().nlinks(); ilink++) {
          if (!pft->hcal.elinks().isActive(ilink)) continue;
          
          int iroc=ilink/2;        
          char pagename[32];
          snprintf(pagename,32,"CHANNEL_%d",(ilink%2)*(NUM_ELINK_CHAN)+ichan);
          // set the value
          pft->hcal.roc(iroc).applyParameter(pagename, modeinfo, 1);
        }

        //////////////////////////////////////////////////////////
        /// Take the expected number of events and save the events
        for (int ievt=0; ievt<events_per_step; ievt++) {

          pft->backend->fc_calibpulse();
          std::vector<uint32_t> event = pft->daqReadEvent();
          pft->backend->fc_advance_l1_fifo();


          // here we decode the event and store the relevant information only...
          pflib::decoding::SuperPacket data(&(event[0]),event.size());

          for (int ilink=0; ilink<pft->hcal.elinks().nlinks(); ilink++) {
            if (!pft->hcal.elinks().isActive(ilink)) continue;

            csv_out << value << ',' << ilink << ',' << ichan << ',' << ievt;
            for (int i=0; i<nsamples; i++) csv_out << ',' << data.sample(i).roc(ilink).get_adc(ichan);
            for (int i=0; i<nsamples; i++) csv_out << ',' << data.sample(i).roc(ilink).get_tot(ichan);
            for (int i=0; i<nsamples; i++) csv_out << ',' << data.sample(i).roc(ilink).get_toa(ichan);

            csv_out<<std::endl;
            /*
            if (ilink==0) {
              data.sample(1).roc(ilink).dump();
            }
            */
          }
        }

        ////////////////////////////////////////////////////////////
        /// Disable charge injection channel by channel -- per elink        
        for (int ilink=0; ilink<pft->hcal.elinks().nlinks(); ilink++) {
          if (!pft->hcal.elinks().isActive(ilink)) continue;
          
          int iroc=ilink/2;        
          char pagename[32];
          snprintf(pagename,32,"CHANNEL_%d",(ilink%2)*(NUM_ELINK_CHAN)+ichan);
          // set the value
          pft->hcal.roc(iroc).applyParameter(pagename, modeinfo, 0);
        }

        //////////////////////////////////////////////////////////
      }
    }
  
    printf(" Diabling IntCTest...\n");
    
    for (int ilink=0; ilink<pft->hcal.elinks().nlinks(); ilink++) {
      if (!pft->hcal.elinks().isActive(ilink)) continue;
      
      int iroc=ilink/2;        
      char pagename[32];
      snprintf(pagename,32,"REFERENCE_VOLTAGE_%d",(ilink%2));
      // set the value
      pft->hcal.roc(iroc).applyParameter(pagename, "INTCTEST", 0);
    }
      ////////////////////////////////////////////////////////////
  }
}

void beamprep(pflib::PolarfireTarget *pft) {
  if (BaseMenu::readline_bool("Load the default beam config? (Recommended)",
                              true)) {
    // -1 to load parameters on all ROCs
    load_parameters(pft, -1);
  }
  static int num_boards{3};
  num_boards = BaseMenu::readline_int(
      "How many boards are connected to this DPM?", num_boards);
  if (!BaseMenu::readline_bool("Skip customizing? (Recommended)", true)) {
    static int SiPM_bias{3784};
    SiPM_bias = BaseMenu::readline_int("SiPM Bias", SiPM_bias);
    set_bias_on_all_connectors(pft, num_boards, false, SiPM_bias);

    static int gain_conv{4};
    gain_conv = BaseMenu::readline_int("Gain conv", gain_conv);
    poke_all_rochalves(pft, "Global_Analog_", "gain_conv", gain_conv, num_boards);

    static int l1offset{85};
    l1offset = BaseMenu::readline_int("L1OFFSET", l1offset);
    poke_all_rochalves(pft, "Digital_Half_", "L1OFFSET", l1offset, num_boards);
  }
  if (BaseMenu::readline_bool("Disable LED bias? (Recommended)", true)) {
    set_bias_on_all_connectors(pft, num_boards, true, 0);
  }
  std::cout << "Fastcontrol settings\n";
  veto_setup(pft);
  std::cout << "DAQ settings\n";
  setup_dma(pft);

  if (BaseMenu::readline_bool("Setup board specific parameters manually?",
                              true)) {
    std::cout << "Setting board specific parameters...\n"
              << " this should probably be removed or at least not used once "
                 "we have board specific yaml files\n";
    static int tot_vref_value{432};
    tot_vref_value = BaseMenu::readline_int("TOT_VREF: ", tot_vref_value);
    poke_all_rochalves(pft, "REFERENCE_VOLTAGE_", "TOT_VREF", tot_vref_value, num_boards);
    static int toa_vref_value{112};
    toa_vref_value = BaseMenu::readline_int("TOA_VREF: ", toa_vref_value);
    poke_all_rochalves(pft, "REFERENCE_VOLTAGE_", "TOA_VREF", toa_vref_value, num_boards);
  }
  if (BaseMenu::readline_bool("Dump current config?", false)) {
    for (int roc_number {0}; roc_number < num_boards; ++roc_number) {
      dump_rocconfig(pft, roc_number);
    }
  }
}
