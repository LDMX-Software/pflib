#include "pftool_tasks.h"


void make_scan_csv_header(std::ofstream& csv_out,
                          const std::string& valuename,
                          const int nsamples) {
  // csv header
  csv_out <<valuename << ",DPM,ILINK,CHAN,EVENT";
  for (int i=0; i<nsamples; i++) csv_out << ",ADC" << i;
  for (int i=0; i<nsamples; i++) csv_out << ",TOT" << i;
  for (int i=0; i<nsamples; i++) csv_out << ",TOA" << i;
  csv_out << ",CAPACITOR_TYPE";
  csv_out<<std::endl;

}

void take_N_calibevents_with_channel(PolarfireTarget* pft,
                                     std::ofstream& csv_out,
                                     const int capacitor_type,
                                     const int nsamples,
                                     const int events_per_step,
                                     const int ichan,
                                     const int value)
{
  //////////////////////////////////////////////////////////
  /// Take the expected number of events and save the events
  for (int ievt=0; ievt<events_per_step; ievt++) {

    pft->backend->fc_calibpulse();
    std::vector<uint32_t> event = pft->daqReadEvent();
    pft->backend->fc_advance_l1_fifo();


    // here we decode the event and store the relevant information only...
    pflib::decoding::SuperPacket data(&(event[0]),event.size());
    const auto dpm {get_dpm_number()};

    for (int ilink=0; ilink<pft->hcal.elinks().nlinks(); ilink++) {
      if (!pft->hcal.elinks().isActive(ilink)) continue;

      csv_out << value << ',' << dpm << ',' << ilink << ',' << ichan << ',' << ievt;
      for (int i=0; i<nsamples; i++) {
        csv_out << ',' << data.sample(i).roc(ilink).get_adc(ichan);
      }
      for (int i=0; i<nsamples; i++) {
        csv_out << ',' << data.sample(i).roc(ilink).get_tot(ichan);
      }
      for (int i=0; i<nsamples; i++) {
       csv_out << ',' << data.sample(i).roc(ilink).get_toa(ichan);
      }
      csv_out << ',' << capacitor_type;

      csv_out<< std::endl;
    }
  }
}

void set_one_channel_per_elink(PolarfireTarget* pft,
                               const std::string& parameter,
                               const int channels_per_elink,
                               const int ichan,
                               const int value)
{

  ////////////////////////////////////////////////////////////
  /// Enable charge injection channel by channel -- per elink
  for (int ilink=0; ilink<pft->hcal.elinks().nlinks(); ilink++) {
    if (!pft->hcal.elinks().isActive(ilink)) continue;

    int iroc=ilink/2;
    const int roc_half = ilink % 2;
    const int channel_number = roc_half * channels_per_elink + ichan;
    // char pagename[32];
    // snprintf(pagename,32,"CHANNEL_%d",(ilink%2)*(channels_per_elink)+ichan);
    // set the value
    const std::string pagename = "CHANNEL_" + std::to_string(channel_number);
    pft->hcal.roc(iroc).applyParameter(pagename, parameter, value);
  }
}

void enable_one_channel_per_elink(PolarfireTarget* pft, const std::string& modeinfo,
                                  const int channels_per_elink,
                                  const int ichan) {
  set_one_channel_per_elink(pft, modeinfo, channels_per_elink, ichan, 1);
}

void disable_one_channel_per_elink(PolarfireTarget* pft, const std::string& modeinfo,
                                   const int channels_per_elink,
                                  const int ichan) {
  set_one_channel_per_elink(pft, modeinfo, channels_per_elink, ichan, 0);
}
void scan_N_steps(PolarfireTarget* pft,
                  std::ofstream& csv_out,
                  const int events_per_step,
                  const int steps,
                  const int low_value,
                  const int high_value,
                  const int nsamples,
                  const std::string& valuename,
                  const std::string& pagetemplate,
                  const std::string& modeinfo)
{
    int capacitor_type {-1};
    if (modeinfo == "HIGHRANGE") {
      capacitor_type = 1;
    } else if (modeinfo == "LOWRANGE") {
      capacitor_type = 0;
    }
    ////////////////////////////////////////////////////////////

    for (int step=0; step<steps; step++) {

      ////////////////////////////////////////////////////////////
      /// set values
      int value=low_value+step*(high_value-low_value)/std::max(1,steps-1);
      printf(" Scan %s=%d...\n",valuename.c_str(),value);

      poke_all_rochalves(pft, pagetemplate, valuename, value);
      ////////////////////////////////////////////////////////////

      pft->prepareNewRun();

      const int channels_per_elink = get_num_channels_per_elink();
      for (int ichan=0; ichan<channels_per_elink; ichan++) {

        enable_one_channel_per_elink(pft, modeinfo, channels_per_elink, ichan);

        take_N_calibevents_with_channel(pft,
                                        csv_out,
                                        capacitor_type,
                                        nsamples,
                                        events_per_step,
                                        ichan,
                                        value);
        csv_out << std::flush;

        disable_one_channel_per_elink(pft, modeinfo, channels_per_elink, ichan);
      }
    }
}

void teardown_charge_injection(PolarfireTarget* pft)
{
  const int num_rocs {get_num_rocs()};
  printf(" Diabling IntCTest...\n");


  std::string intctest_page = "REFERENCE_VOLTAGE_";
  std::string intctest_parameter = "INTCTEST";
  const int intctest = 0;
  std::cout << "Setting " << intctest_parameter << " on page "
            << intctest_page << " to "
            << intctest << '\n';
  poke_all_rochalves(pft, intctest_page, intctest_parameter, intctest);
  std::cout << "Note: L1OFFSET is not reset automatically by charge injection scans\n";

}
void prepare_charge_injection(PolarfireTarget* pft)
{
  const calibrun_hardcoded_values hc{};
  const int num_rocs {get_num_rocs()};
  const int off_value{0};
    printf(" Clearing charge injection on all channels (ground-state)...\n");
    poke_all_channels(pft, "HIGHRANGE", off_value);
    printf("Highrange cleared\n");
    poke_all_channels(pft, "LOWRANGE", off_value);
    printf("Lowrange cleared\n");

    printf(" Enabling IntCTest...\n");

    const int intctest = 1;
    std::cout << "Setting " << hc.intctest_parameter << " on page "
              << hc.intctest_page << " to "
              << intctest << '\n';
    poke_all_rochalves(pft, hc.intctest_page, hc.intctest_parameter, intctest);


    std::cout << "Setting " <<hc.l1offset_parameter << " on page "
              << hc.l1offset_page << " to "
              << hc.charge_l1offset << '\n';
    poke_all_rochalves(pft, hc.l1offset_page, hc.l1offset_parameter, hc.charge_l1offset);
}

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
  if (cmd=="RESET_POWERUP") {
    pft->hcal.fc().resetTransmitter();
    pft->hcal.resyncLoadROC();
    pft->daqHardReset();
    pflib::Elinks& elinks=pft->hcal.elinks();
    elinks.resetHard();
  }

  if (cmd == "CALIBRUN") {
    const std::string pedestal_command{"PEDESTAL"};
    // auto output_directory = BaseMenu::readline("Output directory for data: (Must end with / and exist)",
    //                                            "/home/ldmx/pflib/temporary_until_we_decide_where_to_put_stuff/");
    auto pedestal_filename=BaseMenu::readline("Filename for pedestal run:  ",
                                make_default_daq_run_filename(pedestal_command));
    std::string chargescan_filename=BaseMenu::readline(
      "Filename for charge scan:  ",
      make_default_chargescan_filename(pft, "CALIBRUN"));
    const auto led_filenames {make_led_filenames()};

    calibrun(pft, pedestal_filename, chargescan_filename, led_filenames);


    return;
  }
  if (cmd=="SCANCHARGE") {
    valuename="CALIB_DAC";
    pagetemplate="REFERENCE_VOLTAGE_";
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

    steps=BaseMenu::readline_int("Number of steps?",steps);
    events_per_step=BaseMenu::readline_int("Events per step?",events_per_step);


    std::string fname=BaseMenu::readline("Filename :  ",
                                         make_default_chargescan_filename(pft, valuename));
    std::ofstream csv_out=std::ofstream(fname);
    make_scan_csv_header(csv_out, valuename, nsamples);

    prepare_charge_injection(pft);

    scan_N_steps(pft,
                 csv_out,
                 events_per_step,
                 steps,
                 low_value,
                 high_value,
                 nsamples,
                 valuename,
                 pagetemplate,
                 modeinfo
                 );

    teardown_charge_injection(pft);
    }
}

void beamprep(pflib::PolarfireTarget *pft) {
  if (BaseMenu::readline_bool("Load the default beam configs? (Recommended)",
                              true)) {
    // -1 to load parameters on all ROCs
    load_parameters(pft, -1);
  }
  const int num_boards{ get_num_rocs()};

  std::cout << "There are some options that people might want to test changing"
            << "when developing the beam setup that can be customized here.\n "
            << "However, unless you know what you are doing OR you skipped "
            << "configuration files, you, should probably skip customizing these.\n"
            << "The default values should correspond to the values we want, but record them if you do run this\n";



  static int SiPM_bias{3784};
  SiPM_bias = BaseMenu::readline_int("SiPM Bias", SiPM_bias);
  set_bias_on_all_connectors(pft, num_boards, false, SiPM_bias);
  if (!BaseMenu::readline_bool("Skip customizing? (Recommended)", true)) {

    static int gain_conv{4};
    gain_conv = BaseMenu::readline_int("Gain conv", gain_conv);
    poke_all_rochalves(pft, "Global_Analog_", "gain_conv", gain_conv);

    static int l1offset{83};
    l1offset = BaseMenu::readline_int("L1OFFSET", l1offset);
    poke_all_rochalves(pft, "Digital_Half_", "L1OFFSET", l1offset);
  }
  if (BaseMenu::readline_bool("Disable LED bias? (Recommended)", true)) {
    set_bias_on_all_connectors(pft, num_boards, true, 0);
  }
  std::cout << "Fastcontrol settings\n";
  // BUSY, OCC, Dont ask the user
  veto_setup(pft, true, false,
             BaseMenu::readline_bool("Customize veto setup (Not recommended)", false));
  if(BaseMenu::readline_bool("Customize triger enables? (Not recommended)", false)) {
    fc_enables(pft);
  } else {
    fc_enables(pft, true, true, true);
  }
  std::cout << "DAQ/DMA settings\n";
  setup_dma(pft, true);

  // if (BaseMenu::readline_bool("Setup board specific parameters manually?",
  //                             true)) {
  //   std::cout << "Setting board specific parameters...\n"
  //             << " this should probably be removed or at least not used once "
  //                "we have board specific yaml files\n";
  //   static int tot_vref_value{432};
  //   tot_vref_value = BaseMenu::readline_int("TOT_VREF: ", tot_vref_value);
  //   poke_all_rochalves(pft, "REFERENCE_VOLTAGE_", "TOT_VREF", tot_vref_value, num_boards);
  //   static int toa_vref_value{112};
  //   toa_vref_value = BaseMenu::readline_int("TOA_VREF: ", toa_vref_value);
  //   poke_all_rochalves(pft, "REFERENCE_VOLTAGE_", "TOA_VREF", toa_vref_value, num_boards);
  // }
  std::cout << "DAQ status:\n";
  daq_status(pft);
  if (BaseMenu::readline_bool("Dump current config?", false)) {
    for (int roc_number {0}; roc_number < num_boards; ++roc_number) {
      dump_rocconfig(pft, roc_number);
    }
  }
  daq_status(pft);
}


std::string make_default_led_template() {

    time_t t=time(NULL);
    struct tm *tm = localtime(&t);
    char fname_def_format[1024];
    const int dpm{get_dpm_number()};
    sprintf(fname_def_format,"led_DPM%d_%%Y%%m%%d_%%H%%M%%S_dac_", dpm);
    char fname_def[1024];
    strftime(fname_def, sizeof(fname_def), fname_def_format, tm);
    return fname_def;
}

std::string make_default_chargescan_filename(PolarfireTarget* pft,
                                             const std::string& valuename)
{
    int len{};
    int offset{};
    pft->backend->fc_get_setup_calib(len,offset);
    time_t t=time(NULL);
    struct tm *tm = localtime(&t);
    char fname_def_format[1024];
    sprintf(fname_def_format,"scan_%s_coff%d_%%Y%%m%%d_%%H%%M%%S.csv",valuename.c_str(), offset);
    char fname_def[1024];
    strftime(fname_def, sizeof(fname_def), fname_def_format, tm);
    return fname_def;
}


std::vector<std::string> make_led_filenames() {
  const calibrun_hardcoded_values hc{};
    std::string led_filename_template = BaseMenu::readline(
      "Filename template for LED runs (dac value is appended to this):",
      make_default_led_template());
    std::cout << "Performing LED runs with DAC values: \n";
    std::vector<std::string> led_filenames{};

    for (auto dac_value : hc.led_dac_values) {
      std::string filename = led_filename_template + std::to_string(dac_value) + ".raw";
      std::cout << dac_value << ": " << filename <<'\n';
      led_filenames.push_back(filename);
    }
    return led_filenames;
}

void calibrun_ledruns(pflib::PolarfireTarget* pft,
                     const std::vector<std::string>& led_filenames) {
  const calibrun_hardcoded_values hc{};

  std::cout << "Setting up fc->led_calib_pulse with length "
            << hc.led_calib_length
            << " and offset "
            << hc.led_calib_offset
            <<'\n';
  fc_calib(pft, hc.led_calib_length, hc.led_calib_offset);
  const int num_boards{get_num_rocs()};
  std::cout << "Setting SiPM bias to " << hc.SiPM_bias << " on all boards in case it was disabled for the charge injection runs\n";
  set_bias_on_all_connectors(pft, num_boards, false, hc.SiPM_bias);


    // IntCtest should be off
    // Set Bias
    // Run calib pulse
    const std::string led_command = "CHARGE";
    const int rate = 100;
    const int run = 0;
    const int num_rocs {get_num_rocs()};
    std::cout << "Setting " << hc.l1offset_parameter << " on page "
              << hc.l1offset_page << " to "
              << hc.led_l1offset << '\n';
    poke_all_rochalves(pft, hc.l1offset_page, hc.l1offset_parameter, hc.led_l1offset);

    for (int i {0}; i < hc.led_dac_values.size(); ++i) {
      const int dac_value {hc.led_dac_values[i]};
      std::cout << "Doing LED run with dac value: " << dac_value <<'\n';
      set_bias_on_all_connectors(pft, num_rocs, true, dac_value);

      pft->prepareNewRun();
      daq_run(pft, led_command, run, hc.num_led_events, rate, led_filenames[i]);
    }

}

void calibrun(pflib::PolarfireTarget* pft,
             const std::string& pedestal_filename,
             const std::string& chargescan_filename,
             const std::vector<std::string>& led_filenames)
{

  header_check(pft, 100);
  const calibrun_hardcoded_values hc{};
  std::cout << "Setting up fc->calib_pulse with length "
            << hc.calib_length
            << " and offset "
            << hc.calib_offset
            <<'\n';
  fc_calib(pft, hc.calib_length, hc.calib_offset);



  std::cout << "Disabling external L1A, external spill, and timer L1A\n";
  fc_enables(pft, false, false, false);

  std::cout << "Disabling DMA\n";
  setup_dma(pft, false);


  const int num_boards {get_num_rocs()};
  const int LED_bias {0};
  std::cout << "Disabling LED bias\n";
  set_bias_on_all_connectors(pft, num_boards, true, LED_bias);

  if (BaseMenu::readline_bool("Enable SiPM bias during the calibration procedure?", true)) {
    std::cout << "Setting SiPM bias to " << hc.SiPM_bias << " on all boards\n";
    set_bias_on_all_connectors(pft, num_boards, false, hc.SiPM_bias);
  } else {
    std::cout << "Disabling SiPM bias on all boards\n";
    set_bias_on_all_connectors(pft, num_boards, false, 0);
  }

  std::cout << "... Done\n";

  std::cout << "DAQ status:\n";
  daq_status(pft);

  std::cout << "Staring pedestal run\n";
  const int rate = 100;
  const int run = 0;
  const int number_of_events = 1000;
  std::cout << number_of_events << " events with rate " << rate << " Hz\n";
  const std::string pedestal_command{"PEDESTAL"};
  pft->prepareNewRun();
  daq_run(pft, pedestal_command, run, number_of_events, rate, pedestal_filename);



  std::ofstream csv_out {chargescan_filename};

  make_scan_csv_header(csv_out, "CALIB_DAC",  hc.nsamples);

  std::cout << "Note: Currently not doing anything with the software veto settings\n";
  std::cout << "Prepare for charge injection\n";
  prepare_charge_injection(pft);


  std::cout << "Running charge injection with lowrange from "  <<
    hc.lowrange_dac_min << " to " << hc.lowrange_dac_max << "\n";

  const std::string dac_page = "REFERENCE_VOLTAGE_";
  const std::string dac_parameter_name = "CALIB_DAC";
  std::cout << "Scanning " <<  hc.coarse_steps
            << " steps of " << dac_parameter_name << " with "
            << hc.events_per_step << " events per step\n";
  scan_N_steps(pft,
               csv_out,
               hc.events_per_step,
               hc.coarse_steps,
               hc.lowrange_dac_min,
               hc.lowrange_dac_max,
               hc.nsamples,
               dac_parameter_name,
               dac_page,
               "LOWRANGE");

  std::cout << "Running charge injection with highrange from "  <<
    hc.highrange_fine_dac_min << " to " << hc.highrange_fine_dac_max << "\n";

  scan_N_steps(pft,
               csv_out,
               hc.events_per_step,
               hc.fine_steps,
               hc.highrange_fine_dac_min,
               hc.highrange_fine_dac_max,
               hc.nsamples,
               dac_parameter_name,
               dac_page,
               "HIGHRANGE"
  );
  std::cout << "Running charge injection with highrange from "  <<
    hc.highrange_coarse_dac_min << " to " << hc.highrange_coarse_dac_max << "\n";

  scan_N_steps(pft,
               csv_out,
               hc.events_per_step,
               hc.coarse_steps,
               hc.highrange_coarse_dac_min,
               hc.highrange_coarse_dac_max,
               hc.nsamples,
               dac_parameter_name,
               dac_page,
               "HIGHRANGE"
  );

  std::cout << "Running teardown for charge injection\n";
  teardown_charge_injection(pft);
  calibrun_ledruns(pft, led_filenames);
  return;


}
