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
  
  if (cmd=="RESET_POWERUP") {
    pft->hcal.fc().resetTransmitter();
    pft->hcal.resyncLoadROC();
    pft->daqHardReset();
    pflib::Elinks& elinks=pft->hcal.elinks();
    elinks.resetHard();
  }
  
  if (cmd=="SCANCHARGE") {
    valuename="CALIB_DAC";
    pagetemplate="REFERENCE_VOLTAGE_%d";
    /// HGCROCv2 only has 11 bit dac
    printf("CALIB_DAC valid range is 0...2047\n");
    low_value=BaseMenu::readline_int("Smallest value of CALIB_DAC?",low_value);
    high_value=BaseMenu::readline_int("Largest value of CALIB_DAC?",high_value);
    bool is_high_range=BaseMenu::readline_bool("Use HighRange? ",false);
    if (is_high_range) modeinfo="HIGHRANGE";
    else modeinfo="LOWRANGE";
  }
  /// common stuff for all scans
  if (cmd=="SCANCHARGE") {
    static const int NUM_ELINK_CHAN=36;

    steps=BaseMenu::readline_int("Number of steps?",steps);
    events_per_step=BaseMenu::readline_int("Events per step?",events_per_step);

    time_t t=time(NULL);
    struct tm *tm = localtime(&t);
    char fname_def_format[1024];
    sprintf(fname_def_format,"scan_%s_%%Y%%m%%d_%%H%%M%%S.csv",valuename.c_str());
    char fname_def[1024];
    strftime(fname_def, sizeof(fname_def), fname_def_format, tm); 
    
    std::string fname=BaseMenu::readline("Filename :  ", fname_def);
    std::ofstream csv_out=std::ofstream(fname);

    // csv header
    csv_out <<valuename << ",ILINK,CHAN,EVENT";
    for (int i=0; i<nsamples; i++) csv_out << ",ADC" << i;
    for (int i=0; i<nsamples; i++) csv_out << ",TOT" << i;
    csv_out<<std::endl;

    if (cmd=="SCANCHARGE") {
      
    }
    
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
  if(cmd == "READ_PEDESTAL"){
    pft->backend->fc_sendL1A();
    std::vector<uint32_t> event = pft->daqReadEvent();

    pflib::decoding::SuperPacket data(&(event[0]),event.size());

    int iroc=0;
    iroc=BaseMenu::readline_int("Which ROC:",iroc);
    
    for(int ilink = iroc*2; ilink <= iroc*2+1; ilink++){
      if (pft->hcal.elinks().isActive(ilink)) {
        for (int k=0; k < 36; k++){
          std::cout << "Ch " << k + (ilink % 2)*36<< ":  ";
          for (int i=0; i<nsamples; i++){
            std::cout << ' ' << data.sample(i).roc(ilink).get_adc(k);
          }
          std::cout << std::endl;
        }
      }
      else{
        std::cout << "Link not active" << std::endl;
      }
    }
  }

  if(cmd == "PREAMP_ALIGNMENT"){
    std::cout << "The pre-amp pedestal alignment is best done when gain_conv = 0" << std::endl;

    static const int NUM_ELINK_CHAN=36;

    int iroc=0;
    iroc=BaseMenu::readline_int("Which ROC:",iroc);
    
    std::cout << "Setting channel-wise ref_dac_inv to 0" << std::endl;
 
    for (int ilink=iroc*2; ilink <= iroc*2+1; ilink++) {
      if (!pft->hcal.elinks().isActive(ilink)) continue;

      for (int ichan=0; ichan<NUM_ELINK_CHAN; ichan++) {
        char pagename[32];
        snprintf(pagename,32,"CHANNEL_%d",(ilink%2)*(NUM_ELINK_CHAN)+ichan);
        pft->hcal.roc(iroc).applyParameter(pagename, "REF_DAC_INV", 0);
      }
    }

    //Choose goal for pedestal
    std::cout << "The channel-wise pre-amplifier voltage ref_dac_inv can only raise pedestals" << std::endl;
    int goal=0;
    goal=BaseMenu::readline_int("Raise pedestals to:",goal);

    int previous[72];
    int stop[72];

    //Try values 0 - 31 of ref_dac_inv
    for(int ref_dac_inv = 0; ref_dac_inv < 32; ref_dac_inv++){
      std::cout << ref_dac_inv << std::endl;

      pft->backend->fc_sendL1A();
      std::vector<uint32_t> event = pft->daqReadEvent();
      pflib::decoding::SuperPacket data(&(event[0]),event.size());

      //Go through both chip halfs
      for(int ilink = iroc*2; ilink <= iroc*2+1; ilink++){
        if (pft->hcal.elinks().isActive(ilink)) {
          for (int k=0; k < 36; k++){
            if(ref_dac_inv == 0){
              previous[k+(ilink % 2)*36] = 0;
              stop[k+(ilink % 2)*36] = 0;
            }

            float avg = 0;
            for (int i=0; i<nsamples; i++){
              avg += data.sample(i).roc(ilink).get_adc(k);
            }
            avg /= nsamples;

            if((avg < (goal-10)) && stop[k+(ilink % 2)*36] == 0){
              char pagename[32];
              snprintf(pagename,32,"CHANNEL_%d",k+(ilink % 2)*36);
              pft->hcal.roc(iroc).applyParameter(pagename, "REF_DAC_INV", ref_dac_inv);
              previous[k+(ilink % 2)*36] = ref_dac_inv;
            }
            if((avg > goal+10) && stop[k+(ilink % 2)*36] == 0){
              char pagename[32];
              snprintf(pagename,32,"CHANNEL_%d",k+(ilink % 2)*36);
              pft->hcal.roc(iroc).applyParameter(pagename, "REF_DAC_INV", previous[k+(ilink % 2)*36]);
              stop[k+(ilink % 2)*36] = 1;
            } 
          }
        }
      }
    }
  }
}
