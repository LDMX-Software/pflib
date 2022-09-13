#include "pftool_tasks.h"
#include "pftool_elinks.h"
#include "pftool_roc.h"
#include <cstdint>

// Really wishing we had C++17 here for the portable filesystem library...
// https://stackoverflow.com/questions/18100097/portable-way-to-check-if-directory-exists-windows-linux-c
//
// Not really handling any errors, just report back true/false
bool directory_exists(const std::string& directory) {

  struct stat info;
  // Populates the "info" struct
  int statRC = stat(directory.c_str(), &info);
  if (statRC != 0) {
    return false;
  }
  return (info.st_mode & S_IFDIR) ? 1 : 0;
}

std::string get_yearmonthday()
{
  // Time since epoch
  std::time_t time_raw{};
  // Gets current time since epoch
  std::time(&time_raw);
  // Convert ot local time in a struct that holds relevant calendar components
  std::tm* time_info = std::localtime(&time_raw);
  // Overkill
  constexpr int buffer_size {2048};
  char buffer[buffer_size];

  // %Y -> Year in four digits
  // %m -> Month in year in two digits
  // %d -> Day in month in two digits (i.e. including 0 for 01 etc)
  // buffer will be null terminated
  std::strftime(buffer, buffer_size, "%Y%m%d", time_info);

  // Constructs an std::string
  return buffer;
}

std::string get_output_directory()
{

  const std::string yearmonthday =  get_yearmonthday();
  const std::string default_output_directory = "./data/" + yearmonthday + "/";
  static std::string output_directory = BaseMenu::readline(
    "Output directory for calibration data: ",
    default_output_directory);

  while (! directory_exists(output_directory)) {
    std::cout << output_directory << " does not exist. Please enter a new one or create it and re-enter the current option.";
      output_directory = BaseMenu::readline("Output directory for calibration data",
                                                        output_directory);
  }
  return output_directory;

}


double get_average_adc(pflib::PolarfireTarget* pft,
                       const pflib::decoding::SuperPacket& data,
                       const int link,
                       const int ch)
{

  const int nsamples = get_number_of_samples_per_event(pft);
  double channel_average {};
  for (int sample{0}; sample < nsamples; ++sample) {
    channel_average += data.sample(sample).link(link).get_adc(ch);
  }
  channel_average /= nsamples;
  return channel_average;
}

std::vector<double> get_pedestal_stats(pflib::PolarfireTarget*pft,
                                       const pflib::decoding::SuperPacket& data,
                                       const int link)
{

  constexpr const int num_channels = 36;
  std::vector<double> averages{};
  for (int ch = 0; ch < num_channels; ++ch) {
    averages.push_back(get_average_adc(pft, data, link, ch));
  }
  auto average = std::accumulate(std::begin(averages),
                                 std::end(averages),
                                 0.) / num_channels;
  auto sum_squared = std::inner_product(std::begin(averages),
                                        std::end(averages),
                                        std::begin(averages),
                                        0.);
  auto std_dev = std::sqrt(sum_squared/num_channels - average * average);

  auto minmax = std::minmax_element(std::begin(averages), std::end(averages));
  return {average, std_dev,*(minmax.first), *(minmax.second)};
}
std::vector<double> get_pedestal_stats(pflib::PolarfireTarget* pft)
{

  static int iroc=0;
  iroc=BaseMenu::readline_int("Which ROC:",iroc);
  const int nsamples = get_number_of_samples_per_event(pft);
  static int half = 0;
  half = BaseMenu::readline_int("Which ROC half? 0/1", half);
  const int link = iroc * 2 + half;
  pft->prepareNewRun();

  pft->backend->fc_sendL1A();
  std::vector<uint32_t> event = pft->daqReadEvent();
  pflib::decoding::SuperPacket data(&(event[0]),event.size());
  return get_pedestal_stats(pft, data, link);
}


void test_dacb_one_channel_at_a_time(pflib::PolarfireTarget* pft)
{

  static int iroc=0;
  iroc=BaseMenu::readline_int("Which ROC:",iroc);
  const int nsamples = get_number_of_samples_per_event(pft);
  static int half = 0;
  half = BaseMenu::readline_int("Which ROC half? 0/1", half);
  static int channel = 35;
  static int signdac = 0;
  static int dacb = 60;

  const int link = iroc * 2 + half;
  auto roc {pft->hcal.roc(iroc)};
  const std::string dacb_parameter = "DACB";
  const std::string signdac_parameter = "SIGN_DAC";
  static int num_adc_tests{5};
  do {
    channel = BaseMenu::readline_int("Which Channel to investigate?", channel);
    num_adc_tests = BaseMenu::readline_int("How many pedestal samples to look at?", num_adc_tests);
    const std::string page = "CHANNEL_" + std::to_string(channel + 36 * half);
    do {
      signdac = BaseMenu::readline_int("Signdac? 0 off 1 on", signdac);
      dacb = BaseMenu::readline_int("DACB value", dacb);
      roc.applyParameter(page, dacb_parameter, dacb);
      roc.applyParameter(page, signdac_parameter, signdac);

      for (int i {0}; i < num_adc_tests; ++ i) {
        pft->prepareNewRun();
        pft->backend->fc_sendL1A();
        std::vector<uint32_t> event = pft->daqReadEvent();
        pflib::decoding::SuperPacket data(&(event[0]),event.size());
        std::vector<int> adcs{};
        for (int sample {0}; sample < 8; ++sample) {
          const auto adc = data.sample(sample).link(link).get_adc(channel);
          adcs.push_back(adc);
          std::cout << adc << ", ";
        }
        std::cout << std::endl;
      }
    } while (BaseMenu::readline_bool("Continue trying with this channel?", true));
  } while (BaseMenu::readline_bool("Continue trying with a different channel?", true));



}
void preamp_alignment(PolarfireTarget* pft)
{


  static int iroc=0;
  iroc=BaseMenu::readline_int("Which ROC:",iroc);
  const int nsamples = get_number_of_samples_per_event(pft);
  static int half = 0;
  half = BaseMenu::readline_int("Which ROC half? 0/1", half);


  auto roc {pft->hcal.roc(iroc)};
  if (BaseMenu::readline_bool("Update Inv_vref?", false)){
    static int inv_vref = 425;
    inv_vref = BaseMenu::readline_int("Update Inv_Vref? ", inv_vref);
    const std::string page = "Reference_voltage_" + std::to_string(half);
    const std::string parameter = "Inv_Vref";
    const int value = inv_vref;
    std::cout << "Updating: " << parameter
              << " on page " << page
              << " to value: " << value
              << std::endl;
    roc.applyParameter(page, parameter, value);
  }


  std::cout << "The pre-amp pedestal alignment is best done when gain_conv = 0" << std::endl;
  if (BaseMenu::readline_bool("Update gain conv?", false)) {
    const std::string page = "Global_Analog_" + std::to_string(half);
    const std::string parameter = "gain_conv";
    const int value {BaseMenu::readline_int("Gain_Conv value? ", 0)};
    std::cout << "Updating: " << parameter
              << " on page " << page
              << " to value: " << value
              << std::endl;
    roc.applyParameter(page, parameter, value);

  }
  std::cout << "The pre-amp pedestal alignment is with SiPM bias enabled... " << std::endl;
  if (BaseMenu::readline_bool("Update SiPM bias?", false)) {
    const int num_boards {get_num_rocs()};
    const int SiPM_bias {3784};
    set_bias_on_all_boards(pft, num_boards, false, SiPM_bias);
  }

  //Choose goal for pedestal
  std::cout << "The channel-wise pre-amplifier voltage ref_dac_inv can only raise pedestals" << std::endl;
  static int goal=50;
  goal=BaseMenu::readline_int("Raise pedestals to:",goal);
  static int tolerance = 10;
  tolerance = BaseMenu::readline_int("Tolreance (+/-): ", tolerance);
  std::cout << "Setting channel-wise ref_dac_inv to 0" << std::endl;

  static int dac_start_value = 10;
  dac_start_value = BaseMenu::readline_int("Start from DAC value: ", dac_start_value);


  const int channels_per_elink = get_num_channels_per_elink();
  const std::string parameter = "REF_DAC_INV";
  for (int ch = 0; ch < channels_per_elink; ++ch) {
    const int channel_number = ch + 36 * half;
    const std::string page = "CHANNEL_" + std::to_string(channel_number);
    const int value  = dac_start_value;
    std::cout << "Updating: " << parameter
              << " on page " << page
              << " to value: " << value
              << std::endl;
    roc.applyParameter(page, parameter, value);
  }


  constexpr const int num_channels = 36;
  std::array<int, num_channels> previous{};
  std::array<int, num_channels> stop{};
  std::array<int, num_channels> averages{};

  const int link = iroc * 2 + half;
  std::cout << "ROC: " << iroc << ", half: " << half << ", Link: " << link << std::endl;
  std::cout << "Trying 0 - 31 of REF_DAC_INV... "  <<std::endl;


  std::cout << std::boolalpha;
  auto is_below_tolerance = [&](const double average){
    auto res (average < goal - tolerance);
    // std::cout << average << " is less than tolerance? " << res << std::endl;
    return res;
  };
  auto is_above_tolerance = [&](const double average){
    return (average > goal + tolerance) ;
  };
  auto is_within_tolerance = [&](const double average){
    auto res {!is_below_tolerance(average) && !(is_above_tolerance(average))};
    // std::cout << "Average: " << average << " is between tolerance? " << res << std::endl;
    return res;
  };

  constexpr const int ceiling_5bits {32};
  for (int ref_dac_inv = dac_start_value; ref_dac_inv < ceiling_5bits; ++ref_dac_inv) {
    std::cout << "DAC: " << ref_dac_inv << std::endl;
    pft->prepareNewRun();

    pft->backend->fc_sendL1A();
    std::vector<uint32_t> event = pft->daqReadEvent();
    pflib::decoding::SuperPacket data(&(event[0]),event.size());
    for (int ch = 0 ; ch < num_channels; ++ch) {

      const int channel_number = ch + 36 * half;
      const std::string page = "CHANNEL_" + std::to_string(channel_number);
      // Calculate average over samples for this channel
      auto channel_average {get_average_adc(pft, data, link, ch)};

      if(is_below_tolerance(channel_average)&& stop[ch] != 1) {
        pft->hcal.roc(iroc).applyParameter(page, parameter, ref_dac_inv);
        previous[ch] = ref_dac_inv;
        averages[ch] = channel_average;
      }

      if (is_within_tolerance(channel_average) && stop[ch] != 1) {
        pft->hcal.roc(iroc).applyParameter(page, parameter, previous[ch]);
        averages[ch] = channel_average;
        std::cout << "Channel: " << ch << " succeeded, stopping!" << std::endl;
        stop[ch] = 1;
      }
    }
  }
  for (int ch {0}; ch < num_channels; ++ch) {
    std::cout << "Ch " << ch << ": REF_DAC_INV ->" << previous[ch]
              << ", succeeded? " << (stop[ch] != 0)
              << ", Average ADC: " << averages[ch]
              << std::endl;
  }
  for (int ch{0}; ch < num_channels; ++ ch) {
    if(!stop[ch]) {
      std::cout << "Channel: " << ch << " failed :( " << std::endl;
    }

  }
  return;
}



void read_samples(PolarfireTarget* pft, const pflib::decoding::SuperPacket& data) {
  const int nsamples = get_number_of_samples_per_event(pft);
  static int iroc=0;
  iroc=BaseMenu::readline_int("Which ROC:",iroc);
  static int half = 0;
  half = BaseMenu::readline_int("Which half? 0/1", half);
  static int channel {-1};
  channel = BaseMenu::readline_int("Which channel ? (-1 for all)", channel);


  const int link = iroc * 2 + half;
  std::cout << "ROC: " << iroc << ", half: " << half << ", Link: " << link << std::endl;
  if (channel == -1) {
    for( int ch=0; ch < 36; ++ch) {
      const int channel_number = ch + half * 36;
      std::cout << "Ch: " << ch << ": ";
      for (int sample {0}; sample < nsamples; ++sample) {
        std::cout << ' ' << data.sample(sample).link(link).get_adc(ch);
      }
      std::cout << std::endl;
    }
  } else {
    const int channel_number = channel + half * 36;
    std::cout << "Ch: " << channel << ": ";
    for (int sample {0}; sample < nsamples; ++sample) {
      std::cout << ' ' << data.sample(sample).link(link).get_adc(channel);
    }
    std::cout << std::endl;
  }

  auto stats {get_pedestal_stats(pft, data, link) };
  std::cout << "Average: " << stats[0] << " sigma, " << stats[1]
            <<", min " << stats[2] << ", max " << stats[3]
            << ", Delta min/max " << stats[3] - stats[2] <<std::endl;

}

void read_charge(PolarfireTarget* pft) {
  pft->prepareNewRun();
  pft->backend->fc_calibpulse();
  std::vector<uint32_t> event {pft->daqReadEvent()};
  pflib::decoding::SuperPacket data{&(event[0]), static_cast<int>(event.size())};
  pft->backend->fc_advance_l1_fifo();
  read_samples(pft, data);
}
void read_pedestal(PolarfireTarget* pft)
{

  pft->prepareNewRun();
  pft->backend->fc_sendL1A();
  std::vector<uint32_t> event = pft->daqReadEvent();
  pflib::decoding::SuperPacket data(&(event[0]),event.size());
  read_samples(pft, data);

}

void make_scan_csv_header(PolarfireTarget* pft,
                          std::ofstream& csv_out,
                          const std::string& valuename) {

  const int nsamples = get_number_of_samples_per_event(pft);
  // csv header
  if(valuename == "GET_TOT"){
    csv_out <<"CALIB_DAC," << "TOT_VREF," << "REF_TOT_DAC" << ",DPM,ILINK,CHAN,EVENT";
    for (int i=0; i<nsamples; i++) csv_out << ",ADC" << i;
    for (int i=0; i<nsamples; i++) csv_out << ",TOT" << i;
    for (int i=0; i<nsamples; i++) csv_out << ",TOA" << i;
    csv_out<<std::endl;
  }
  else if (valuename == "PHASE_LED"){
    csv_out <<valuename << ",DPM,ILINK,CHAN,EVENT";
    for (int i=0; i<nsamples; i++) csv_out << ",ADC" << i;
    for (int i=0; i<nsamples; i++) csv_out << ",TOT" << i;
    for (int i=0; i<nsamples; i++) csv_out << ",TOA" << i;
    csv_out << ",DAC,LED_BIAS,SIPM_BIAS";
    csv_out<<std::endl;
  }
  else {
    // CALIB_DAC
    csv_out <<valuename << ",DPM,ILINK,CHAN,EVENT";
    for (int i=0; i<nsamples; i++) csv_out << ",ADC" << i;
    for (int i=0; i<nsamples; i++) csv_out << ",TOT" << i;
    for (int i=0; i<nsamples; i++) csv_out << ",TOA" << i;
    csv_out << ",CAPACITOR_TYPE,SIPM_BIAS";
    csv_out<<std::endl;
  }
}
void take_N_calibevents_with_channel(PolarfireTarget* pft,
                                     std::ofstream& csv_out,
                                     const int SiPM_bias,
                                     const int capacitor_type,
                                     const int events_per_step,
                                     const int ichan,
                                     const int value)
{
  const int nsamples = get_number_of_samples_per_event(pft);
  //////////////////////////////////////////////////////////
  /// Take the expected number of events and save the events
  for (int ievt=0; ievt<events_per_step; ievt++) {

    pft->backend->fc_calibpulse();
    std::vector<uint32_t> event = pft->daqReadEvent();
    pft->backend->fc_advance_l1_fifo();

    // here we decode the event and store the relevant information only...
    pflib::decoding::SuperPacket data(&(event[0]),event.size());
    const auto dpm {get_dpm_number(pft)};

    for (int ilink=0; ilink<pft->hcal.elinks().nlinks(); ilink++) {
      if (!pft->hcal.elinks().isActive(ilink)) continue;

      csv_out << value << ',' << dpm << ',' << ilink << ',' << ichan << ',' << ievt;
      for (int i=0; i<nsamples; i++) {
        csv_out << ',' << data.sample(i).link(ilink).get_adc(ichan);
      }
      for (int i=0; i<nsamples; i++) {
        csv_out << ',' << data.sample(i).link(ilink).get_tot(ichan);
      }
      for (int i=0; i<nsamples; i++) {
       csv_out << ',' << data.sample(i).link(ilink).get_toa(ichan);
      }
      csv_out << ',' << capacitor_type << ',' << SiPM_bias;

      csv_out<< '\n';
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
                  const int SiPM_bias,
                  const int events_per_step,
                  const int steps,
                  const int low_value,
                  const int high_value,
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
      std::cout << " Scan " << valuename << "=" << value << "..." << std::endl;

      poke_all_rochalves(pft, pagetemplate, valuename, value);
      ////////////////////////////////////////////////////////////

      pft->prepareNewRun();

      const int channels_per_elink = get_num_channels_per_elink();
      for (int ichan=0; ichan<channels_per_elink; ichan++) {
        std::cout << "C: " << ichan << "... " << std::flush;

        enable_one_channel_per_elink(pft, modeinfo, channels_per_elink, ichan);

        take_N_calibevents_with_channel(pft,
                                        csv_out,
                                        SiPM_bias,
                                        capacitor_type,
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
  std::cout << "Disabling IntCTest" << std::endl;


  std::string intctest_page = "REFERENCE_VOLTAGE_";
  std::string intctest_parameter = "INTCTEST";
  const int intctest = 0;
  std::cout << "Setting " << intctest_parameter << " on page "
            << intctest_page << " to "
            << intctest << std::endl;
  poke_all_rochalves(pft, intctest_page, intctest_parameter, intctest);
  std::cout << "Note: L1OFFSET is not reset automatically by charge injection scans" << std::endl;

}
void prepare_charge_injection(PolarfireTarget* pft)
{
  const calibrun_hardcoded_values hc{};
  const int num_rocs {get_num_rocs()};
  const int off_value{0};
  std::cout << " Clearing charge injection on all channels (ground-state)..." << std::endl;
    poke_all_channels(pft, "HIGHRANGE", off_value);
    std::cout << "Highrange cleared" << std::endl;
    printf("Highrange cleared\n");
    poke_all_channels(pft, "LOWRANGE", off_value);
    std::cout << "Lowrange cleared" << std::endl;

    std::cout << " Enabling IntCTest..." << std::endl;

    const int intctest = 1;
    std::cout << "Setting " << hc.intctest_parameter << " on page "
              << hc.intctest_page << " to "
              << intctest << std::endl;
    poke_all_rochalves(pft, hc.intctest_page, hc.intctest_parameter, intctest);


    std::cout << "Setting " <<hc.l1offset_parameter << " on page "
              << hc.l1offset_page << " to "
              << hc.charge_l1offset << std::endl;
    poke_all_rochalves(pft, hc.l1offset_page, hc.l1offset_parameter, hc.charge_l1offset);
}

void phasescan(PolarfireTarget* pft) {

  std::cout << "Running DAQ softreset" <<std::endl;
  daq_softreset(pft);
  std::cout << "Running DAQ standard setup" << std::endl;
  daq_standard(pft);
  std::cout << "Running DAQ enable" << std::endl;
  daq_enable(pft);
  const int nsamples = get_number_of_samples_per_event(pft);
  std::cout << "Disabling external L1A, external spill, and timer L1A" << std::endl;
  fc_enables(pft, false, false, false);

  std::cout << "Disabling DMA" << std::endl;
  setup_dma(pft, false);

  const bool flashLEDs{BaseMenu::readline_bool("Scan with LED? If false, will use charge injection.", false)};

  if (flashLEDs) {
    std::cout << "Doing phase scan with LED flash..." << std::endl;
    if (BaseMenu::readline_bool("Update bias settings?", false)) {
      const int SiPM_bias{BaseMenu::readline_int("SiPM bias: ", 3784)};
      const int LED_bias{BaseMenu::readline_int("LED bias: ", 1500)};
      set_bias_on_all_active_boards(pft, true, LED_bias);
      set_bias_on_all_active_boards(pft, false, SiPM_bias);
    }
    static int LED_dac {1500};
    LED_dac = BaseMenu::readline_int("LED DAC (0...2047)", LED_dac);
    const int rate = 100;
    const int run = 0;
    const int events_per_step =
        BaseMenu::readline_int("Number of events per step?", 2);
    std::ofstream csv_out{"PHASESCAN.csv"};
    make_scan_csv_header(pft, csv_out, "PHASE_LED");
    const int steps{
        BaseMenu::readline_int("Number of phase steps: (0 .. 15) ", 15)};
    const std::string page{"TOP"};
    const std::string parameter{"PHASE"};
    const std::vector<int> active_boards{get_rocs_with_active_links(pft)};
    auto active_links {getActiveLinkNumbers(pft)};
    const auto channels_per_elink {get_num_channels_per_elink()};
    const int nsamples = get_number_of_samples_per_event(pft);
  } else {
    std::cout << "Doing phase scan with charge injection...\n";
    std::cout << "Disabling LED bias" << std::endl;
    const int LED_bias{0};
    set_bias_on_all_active_boards(pft, true, LED_bias);
    std::cout << "Setting SiPM bias to: " << SiPM_bias << std::endl;
    set_bias_on_all_active_boards(pft, false, SiPM_bias);

    const int calib_dac{BaseMenu::readline_int("CALIB_DAC (0...2047)", 1700)};
    poke_all_rochalves(pft, "REFERENCE_VOLTAGE_", "CALIB_DAC", calib_dac);
    const int rate = 100;
    const int run = 0;
    const int events_per_step =
        BaseMenu::readline_int("Number of events per step?", 2);
    std::ofstream csv_out{"PHASESCAN.csv"};
    make_scan_csv_header(pft, csv_out, "PHASE");
    prepare_charge_injection(pft);

    const std::string modeinfo{BaseMenu::readline(
        "Which capacitor? (LOWRANGE/HIGHRANGE)", "LOWRANGE")};
    int capacitor_type{-1};
    if (modeinfo == "HIGHRANGE") {
      capacitor_type = 1;
    } else if (modeinfo == "LOWRANGE") {
      capacitor_type = 0;
    }
    const int steps{
        BaseMenu::readline_int("Number of phase steps: (0 .. 15) ", 15)};
    const std::string page{"TOP"};
    const std::string parameter{"PHASE"};
    const std::vector<int> active_boards{get_rocs_with_active_links(pft)};
    for (int step{0}; step < steps; ++step) {
      const int phase{step};
      std::cout << "Scanning phase: " << phase << std::endl;
      for (const auto board : active_boards) {
        auto roc{pft->hcal.roc(board)};
        roc.applyParameter(page, parameter, phase);
      }
      pft->prepareNewRun();
      const int channels_per_elink = get_num_channels_per_elink();
      for (int ichan = 0; ichan < channels_per_elink; ichan++) {
        std::cout << "C: " << ichan << "... " << std::flush;
        enable_one_channel_per_elink(pft, modeinfo, channels_per_elink, ichan);
        take_N_calibevents_with_channel(pft, csv_out, SiPM_bias, capacitor_type,
                                        events_per_step, ichan, phase);
        csv_out << std::flush;
        disable_one_channel_per_elink(pft, modeinfo, channels_per_elink, ichan);
      }
      std::cout << std::endl;
    }
    teardown_charge_injection(pft);
  }
}
void tot_tune(PolarfireTarget* pft)
{

  static int iroc=0;
  iroc=BaseMenu::readline_int("Which ROC:",iroc);
  auto roc {pft->hcal.roc(iroc)};
  const int nsamples = get_number_of_samples_per_event(pft);
  static int half = 0;
  half = BaseMenu::readline_int("Which ROC half? 0/1", half);
  int link = 2*iroc + half;

  std::string valuename="CALIB_DAC";
  std::string totvaluename="TOT_VREF";
  std::string finetotvaluename="REF_DAC_TOT";

  double goal;
  goal=BaseMenu::readline_int("TOT turn-on value? (ADC value):",goal);

  int low_value = 0;
  int high_value = 1023;
  int steps = 10;
  int events_per_step = 100;

  int tot_low_value = 0;
  int tot_high_value = 1023;
  int tot_steps = 10;

  int fine_tot_low_value = 0;
  int fine_tot_high_value = 31;
  int fine_tot_steps = 4;

  std::string modeinfo;

  /// HGCROCv2 only has 11 bit dac
  printf("CALIB_DAC valid range is 0...2047\n");
  low_value=BaseMenu::readline_int("Smallest value of CALIB_DAC?",low_value);
  high_value=BaseMenu::readline_int("Largest value of CALIB_DAC?",high_value);
  bool is_high_range=BaseMenu::readline_bool("Use HighRange? ",false);
  if (is_high_range) modeinfo="HIGHRANGE";
  else modeinfo="LOWRANGE";
  steps=BaseMenu::readline_int("Number of calib_daq steps?",steps);
  events_per_step=BaseMenu::readline_int("Events per step?",events_per_step);

  printf("TOT_VREF valid range is 0...1023\n");
  tot_low_value=BaseMenu::readline_int("Smallest value of TOT_VREF?",tot_low_value);
  tot_high_value=BaseMenu::readline_int("Largest value of TOT_VREF?",tot_high_value);
  tot_steps=BaseMenu::readline_int("Number of TOT_VREF steps?",tot_steps);

  printf("REF_DAC_TOT valid range is 0...31\n");
  fine_tot_low_value=BaseMenu::readline_int("Smallest value of REF_DAC_TOT?",fine_tot_low_value);
  fine_tot_high_value=BaseMenu::readline_int("Largest value of REF_DAC_TOT?",fine_tot_high_value);
  fine_tot_steps=BaseMenu::readline_int("Number of REF_DAC_TOT steps?",fine_tot_steps);

  const auto dpm {get_dpm_number()};

  std::string fname=BaseMenu::readline("Filename :  ",
                                       make_default_chargescan_filename(pft, dpm, valuename, -1));
  std::ofstream csv_out=std::ofstream(fname);
  make_scan_csv_header(pft, csv_out, "GET_TOT");

  int bad_channels[5] = {8,17,26,35,19};

  std::cout << "Performing global tune of TOT_VREF" << std::endl;

  double global_average=1e6;
  double global_rms=1e6;
  int global_tot_vref=0;

  const int channels_per_elink = 36;

  prepare_charge_injection(pft);

  for(int totstep=0;totstep<tot_steps;totstep++){
    int totvalue=tot_low_value+totstep*(tot_high_value-tot_low_value)/std::max(1,tot_steps-1);
    const std::string page = "REFERENCE_VOLTAGE_" + std::to_string(half);
    const std::string parameter = "TOT_VREF";
    std::cout << "Updating: " << parameter
              << " on page " << page
              << " to value: " << totvalue
              << std::endl;
    roc.applyParameter(page, parameter, totvalue);

    double temp_cumul=0;
    double temp_counter=0;
    double temp_rms_prim=0;

    for(int step=0;step<steps;step++){

      std::array<int, channels_per_elink> stop{};

      int value=low_value+step*(high_value-low_value)/std::max(1,steps-1);
      const std::string page = "REFERENCE_VOLTAGE_" + std::to_string(half);
      const std::string parameter = "CALIB_DAC";
      roc.applyParameter(page, parameter, value);

      pft->prepareNewRun();

      for (int ichan=0; ichan<channels_per_elink; ichan++) {
        if(stop[ichan] == 1) continue;
        for (int badchan = 0; badchan < 5; badchan++){
          if(ichan == badchan) continue;
        }

        enable_one_channel_per_elink(pft, "LOWRANGE", channels_per_elink, ichan);

        double channel_average = 0;

        for (int ievt=0; ievt<events_per_step; ievt++) {
          pft->backend->fc_calibpulse();
          std::vector<uint32_t> event = pft->daqReadEvent();
          pft->backend->fc_advance_l1_fifo();

          pflib::decoding::SuperPacket data(&(event[0]),event.size());
          const auto dpm {get_dpm_number()};

          csv_out << value << ',' << totvalue << ',' << 0 << ',' << dpm << ',' << half << ',' << ichan << ',' << ievt;
          for (int i=0; i<nsamples; i++) {
            csv_out << ',' << data.sample(i).link(link).get_adc(ichan);
          }
          for (int i=0; i<nsamples; i++) {
            csv_out << ',' << data.sample(i).link(link).get_tot(ichan);
          }
          for (int i=0; i<nsamples; i++) {
          csv_out << ',' << data.sample(i).link(link).get_toa(ichan);
          }

          csv_out<< '\n';

          for (int i=0; i<nsamples; i++){
            if(data.sample(i).link(link).get_tot(ichan) != 0){
              channel_average += data.sample(i).link(link).get_adc(ichan);
              break;
            }
          }
        }
        if((channel_average) != 0){
          temp_cumul += channel_average/events_per_step;
          temp_counter++;
          temp_rms_prim += pow(channel_average/events_per_step, 2);
          stop[ichan] = 1;
        }
	disable_one_channel_per_elink(pft, "LOWRANGE", channels_per_elink, ichan);
      }
    }
    double temp_average=temp_cumul/temp_counter;
    double temp_rms=sqrt(temp_rms_prim/temp_counter);
    if((abs(temp_average-goal)<global_average) && (temp_rms<global_rms)){
      global_average=abs(temp_average-goal);
      global_rms=temp_rms;
      global_tot_vref=totvalue;
    }
  }

  std::cout << "Performing per_channel tune of REF_DAC_TOT" << std::endl;

  std::array<double, channels_per_elink> per_channel_average{};
  std::array<double, channels_per_elink> per_channel_rms{};
  std::array<int, channels_per_elink> per_channel_dac{};

  const std::string page = "REFERENCE_VOLTAGE_" + std::to_string(half);
  const std::string parameter = "TOT_VREF";
  roc.applyParameter(page, parameter, global_tot_vref);

  prepare_charge_injection(pft);

  for(int finetotstep=0;finetotstep<fine_tot_steps;finetotstep++){
    int finetotvalue=fine_tot_low_value+finetotstep*(fine_tot_high_value-fine_tot_low_value)/std::max(1,fine_tot_steps-1);

    std::array<double, channels_per_elink> per_channel_cumul{};
    std::array<int, channels_per_elink>  per_channel_counter{};
    std::array<double, channels_per_elink> per_channel_rms_prim{};

    for(int step=0;step<steps;step++){

      std::array<int, channels_per_elink> stop{};

      int value=low_value+step*(high_value-low_value)/std::max(1,steps-1);
      const std::string page = "REFERENCE_VOLTAGE_" + std::to_string(half);
      const std::string parameter = "CALIB_DAC";
      roc.applyParameter(page, parameter, value);

      pft->prepareNewRun();

      for (int ichan=0; ichan<channels_per_elink; ichan++) {
        if(stop[ichan] == 1) continue;
        for (int badchan = 0; badchan < 5; badchan++){
          if(ichan == badchan) continue;
        }

        double channel_average = 0;

        enable_one_channel_per_elink(pft, "LOWRANGE", channels_per_elink, ichan);
        set_one_channel_per_elink(pft,"REF_DAC_TOT",channels_per_elink, ichan, finetotvalue);

        for (int ievt=0; ievt<events_per_step; ievt++) {
          pft->backend->fc_calibpulse();
          std::vector<uint32_t> event = pft->daqReadEvent();
          pft->backend->fc_advance_l1_fifo();

          pflib::decoding::SuperPacket data(&(event[0]),event.size());
          const auto dpm {get_dpm_number()};

          csv_out << value << ',' << global_tot_vref << ',' << finetotvalue << ',' << dpm << ',' << half << ',' << ichan << ',' << ievt;
          for (int i=0; i<nsamples; i++) {
            csv_out << ',' << data.sample(i).link(link).get_adc(ichan);
          }
          for (int i=0; i<nsamples; i++) {
            csv_out << ',' << data.sample(i).link(link).get_tot(ichan);
          }
          for (int i=0; i<nsamples; i++) {
          csv_out << ',' << data.sample(i).link(link).get_toa(ichan);
          }

          csv_out<< '\n';

          for (int i=0; i<nsamples; i++){
            if(data.sample(i).link(link).get_tot(ichan) != 0){
              channel_average += data.sample(i).link(link).get_adc(ichan);
              break;
            }
          }
        }
        if((channel_average) != 0){
          per_channel_cumul[ichan] += channel_average/events_per_step;
          per_channel_counter[ichan]++;
          per_channel_rms_prim[ichan] += pow(channel_average/events_per_step, 2);
          stop[ichan] = 1;
        }
	disable_one_channel_per_elink(pft, "LOWRANGE", channels_per_elink, ichan);
      }
    }
    for (int ichan=0; ichan<channels_per_elink; ichan++) {
      for (int badchan = 0; badchan < 5; badchan++){
        if(ichan == badchan) continue;
      }
      double temp_average=per_channel_cumul[ichan]/per_channel_counter[ichan];
      double temp_rms=sqrt(per_channel_rms_prim[ichan]/per_channel_counter[ichan]);
      if((abs(temp_average-goal)<per_channel_average[ichan]) && (temp_rms<per_channel_rms[ichan])){
        per_channel_average[ichan]=abs(temp_average-goal);
        per_channel_rms[ichan]=temp_rms;
        per_channel_dac[ichan]=finetotvalue;
      }
    }
  }
  std::cout << "Best TOT_VREF: " << global_tot_vref << std::endl;
  std::cout << "Average: " << global_average << std::endl;
  std::cout << "RMS: " << global_rms << std::endl;
  for (int ichan=0; ichan<channels_per_elink; ichan++) {
    for (int badchan = 0; badchan < 5; badchan++){
      if(ichan == badchan) continue;
    }
    std::cout << "Best REF_DAC_TOT for Channel" << ichan<< ": " << per_channel_dac[ichan] << std::endl;
    std::cout << "Average: " << per_channel_average[ichan] << std::endl;
    std::cout << "RMS: " << per_channel_rms[ichan] << std::endl;    
  }
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

  const int dpm {get_dpm_number(pft)};
  const int nsamples = get_number_of_samples_per_event(pft);

  if (cmd == "PHASESCAN") {
    phasescan(pft);
    return;
  }

  if (cmd == "TUNE_TOT") {
    tot_tune(pft);
    return;
  }

  if (cmd == "DACB") {
    test_dacb_one_channel_at_a_time(pft);
    return;
  }
  if (cmd == "PEDESTAL_READ") {
    read_pedestal(pft);
    return;
  }
  if (cmd == "CHARGE_READ") {
    read_charge(pft);
    return;
  }
  if (cmd == "ALIGN_PREAMP") {
    preamp_alignment(pft);
    return;
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
    auto pedestal_filename= BaseMenu::readline(
      "Filename for pedestal run:  ",
      make_default_daq_run_filename(pedestal_command, dpm));
    calibrun_hardcoded_values hc{};
    fc_calib(pft, hc.calib_length, hc.calib_offset);
    std::string chargescan_filename=BaseMenu::readline(
      "Filename for charge scan:  ",
      make_default_chargescan_filename(pft, dpm, "CALIBRUN", hc.calib_offset));
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
                                         make_default_chargescan_filename(pft, dpm, valuename, -1));
    std::ofstream csv_out=std::ofstream(fname);
    make_scan_csv_header(pft, csv_out, valuename);

    prepare_charge_injection(pft);
    const int SiPM_bias = BaseMenu::readline_int("SiPM bias: ", 0);
    const int num_boards{get_num_rocs()};
    set_bias_on_all_boards(pft, num_boards, false, SiPM_bias);


    scan_N_steps(pft,
                 csv_out,
                 SiPM_bias,
                 events_per_step,
                 steps,
                 low_value,
                 high_value,
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


  std::cout << "Running DAQ softreset" << std::endl;
  daq_softreset(pft);
  std::cout << "Running DAQ standard setup" << std::endl;
  daq_standard(pft);
  std::cout << "Running DAQ enable" << std::endl;
  daq_enable(pft);

  const calibrun_hardcoded_values hc{};
  std::cout << "Setting number of additional samples to: "
            << hc.num_extra_samples << std::endl;
  const bool enable_multisample = true;
  multisample_setup(pft, enable_multisample, hc.num_extra_samples);

  std::cout << "There are some options that people might want to test changing"
            << "when developing the beam setup that can be customized here.\n "
            << "However, unless you know what you are doing OR you skipped "
            << "configuration files, you, should probably skip customizing these.\n"
            << "The default values should correspond to the values we want, but record them if you do run this\n";



  static int SiPM_bias{3784};
  SiPM_bias = BaseMenu::readline_int("SiPM Bias", SiPM_bias);
  set_bias_on_all_boards(pft, num_boards, false, SiPM_bias);
  if (!BaseMenu::readline_bool("Skip customizing? (Recommended)", true)) {

    static int gain_conv{4};
    gain_conv = BaseMenu::readline_int("Gain conv", gain_conv);
    poke_all_rochalves(pft, "Global_Analog_", "gain_conv", gain_conv);

    static int l1offset{85};
    l1offset = BaseMenu::readline_int("L1OFFSET", l1offset);
    poke_all_rochalves(pft, "Digital_Half_", "L1OFFSET", l1offset);
  }
  if (BaseMenu::readline_bool("Disable LED bias? (Recommended)", true)) {
    set_bias_on_all_boards(pft, num_boards, true, 0);
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

  std::cout << "DAQ status:\n";
  daq_status(pft);
  if (BaseMenu::readline_bool("Dump current config?", true)) {
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
    const std::string filename {get_output_directory() + std::string{fname_def}};
    return filename;
}

std::string make_default_chargescan_filename(PolarfireTarget* pft,
                                             const int dpm,
                                             const std::string& valuename,
                                             const int calib_offset)
{
    int len{};
    int offset{};
    if (calib_offset < 0 && pft != nullptr) {
      pft->backend->fc_get_setup_calib(len,offset);
    } else {
      offset = calib_offset;
    }

    time_t t=time(NULL);
    struct tm *tm = localtime(&t);
    char fname_def_format[1024];
    sprintf(fname_def_format,
            "scan_DPM%d_%s_coff%d_%%Y%%m%%d_%%H%%M%%S.csv",
            dpm,
            valuename.c_str(), offset);
    char fname_def[1024];
    strftime(fname_def, sizeof(fname_def), fname_def_format, tm);
    const auto output_directory{get_output_directory()};
    return output_directory + fname_def;
}


std::vector<std::string> make_led_filenames() {
  const calibrun_hardcoded_values hc{};
    std::string led_filename_template = BaseMenu::readline(
      "Filename template for LED runs (dac value is appended to this):",
      make_default_led_template());
    std::cout << "Performing LED runs with DAC values: " << std::endl;
    std::vector<std::string> led_filenames{};

    for (auto dac_value : hc.led_dac_values) {
      std::string filename = led_filename_template + std::to_string(dac_value) + ".raw";
      std::cout << dac_value << ": " << filename << std::endl;
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
            <<std::endl;
  fc_calib(pft, hc.led_calib_length, hc.led_calib_offset);
  const int num_boards{get_num_rocs()};
  std::cout << "Setting SiPM bias to " << hc.SiPM_bias << " on all boards in case it was disabled for the charge injection runs"  << std::endl;
  set_bias_on_all_boards(pft, num_boards, false, hc.SiPM_bias);


    // IntCtest should be off
    // Set Bias
    // Run calib pulse
    const std::string led_command = "CHARGE";
    const int rate = 100;
    const int run = 0;
    const int num_rocs {get_num_rocs()};
    std::cout << "Setting " << hc.l1offset_parameter << " on page "
              << hc.l1offset_page << " to "
              << hc.led_l1offset << std::endl;
    poke_all_rochalves(pft, hc.l1offset_page, hc.l1offset_parameter, hc.led_l1offset);

    for (int i {0}; i < hc.led_dac_values.size(); ++i) {
      const int dac_value {hc.led_dac_values[i]};
      std::cout << "Doing LED run with dac value: " << dac_value << std::endl;
      set_bias_on_all_boards(pft, num_rocs, true, dac_value);

      pft->prepareNewRun();
      daq_run(pft, led_command, run, hc.num_led_events, rate, led_filenames[i]);
    }

}

void calibrun(pflib::PolarfireTarget* pft,
             const std::string& pedestal_filename,
             const std::string& chargescan_filename,
             const std::vector<std::string>& led_filenames)
{

  const calibrun_hardcoded_values hc{};
  std::cout << "Running DAQ softreset" <<std::endl;
  daq_softreset(pft);
  std::cout << "Running DAQ standard setup" << std::endl;
  daq_standard(pft);
  std::cout << "Running DAQ enable" << std::endl;
  daq_enable(pft);

  std::cout << "Setting number of additional samples to: "
            << hc.num_extra_samples << std::endl;
  const bool enable_multisample = true;
  multisample_setup(pft, enable_multisample, hc.num_extra_samples);

  header_check(pft, 100);



  std::cout << "Disabling external L1A, external spill, and timer L1A" << std::endl;
  fc_enables(pft, false, false, false);

  std::cout << "Disabling DMA" << std::endl;
  setup_dma(pft, false);


  const int num_boards {get_num_rocs()};
  const int LED_bias {0};
  std::cout << "Disabling LED bias" << std::endl;
  set_bias_on_all_boards(pft, num_boards, true, LED_bias);

  std::cout << "Setting SiPM bias to " << hc.SiPM_bias << " on all boards" << std::endl;
  set_bias_on_all_boards(pft, num_boards, false, hc.SiPM_bias);

  std::cout << "... Done" << std::endl;

  std::cout << "DAQ status:" << std::endl;
  daq_status(pft);

  std::cout << "Staring pedestal run" << std::endl;
  const int rate = 100;
  const int run = 0;
  const int number_of_events = 1000;
  std::cout << number_of_events << " events with rate " << rate << " Hz" << std::endl;
  const std::string pedestal_command{"PEDESTAL"};
  pft->prepareNewRun();
  daq_run(pft, pedestal_command, run, number_of_events, rate, pedestal_filename);



  std::ofstream csv_out {chargescan_filename};

  make_scan_csv_header(pft, csv_out, "CALIB_DAC");

  std::cout << "Note: Currently not doing anything with the software veto settings" << std::endl;
  std::cout << "Prepare for charge injection" << std::endl;
  prepare_charge_injection(pft);


  const std::vector<int> SiPM_biases {hc.SiPM_bias};

  for (auto SiPM_bias : SiPM_biases) { //
    std::cout << "Running charge injection for SiPM bias: " << SiPM_bias << std::endl;
    std::cout << "Setting SiPM bias to " << SiPM_bias << " on all boards" << std::endl;
    set_bias_on_all_boards(pft, num_boards, false, SiPM_bias);

    std::cout << "Running charge injection with lowrange from "  <<
      hc.lowrange_dac_min << " to " << hc.lowrange_dac_max << std::endl;

    const std::string dac_page = "REFERENCE_VOLTAGE_";
    const std::string dac_parameter_name = "CALIB_DAC";
    std::cout << "Scanning " <<  hc.coarse_steps
              << " steps of " << dac_parameter_name << " with "
              << hc.events_per_step << " events per step" << std::endl;
    scan_N_steps(pft,
                 csv_out,
                 SiPM_bias,
                 hc.events_per_step,
                 hc.coarse_steps,
                 hc.lowrange_dac_min,
                 hc.lowrange_dac_max,
                 dac_parameter_name,
                 dac_page,
                 "LOWRANGE");

    std::cout << "Running charge injection with highrange from "  <<
      hc.highrange_fine_dac_min << " to " << hc.highrange_fine_dac_max << std::endl;

    scan_N_steps(pft,
                 csv_out,
                 SiPM_bias,
                 hc.events_per_step,
                 hc.fine_steps,
                 hc.highrange_fine_dac_min,
                 hc.highrange_fine_dac_max,
                 dac_parameter_name,
                 dac_page,
                 "HIGHRANGE"
    );
    std::cout << "Running charge injection with highrange from "  <<
      hc.highrange_coarse_dac_min << " to " << hc.highrange_coarse_dac_max << std::endl;

    scan_N_steps(pft,
                 csv_out,
                 SiPM_bias,
                 hc.events_per_step,
                 hc.coarse_steps,
                 hc.highrange_coarse_dac_min,
                 hc.highrange_coarse_dac_max,
                 dac_parameter_name,
                 dac_page,
                 "HIGHRANGE"
    );
  }

  std::cout << "Running teardown for charge injection" << std::endl;
  teardown_charge_injection(pft);
  calibrun_ledruns(pft, led_filenames);
  std::cout << "Reminder:" << std::endl;
  std::cout << "Pedestal filename: " << pedestal_filename << std::endl;
  std::cout << "Charge scan filename: " << chargescan_filename << std::endl;
  for (int i{}; i < hc.led_dac_values.size(); ++i) {
    const int dac_value = hc.led_dac_values[i];
    std::cout << "LED filename for DAC value " << dac_value
              << ": " << led_filenames[i] << std::endl;
  }
  return;


}

namespace {
auto menu_tasks = pftool::menu("TASKS","various high-level tasks like scans and tunes")
//  ->line("RESET_POWERUP", "Execute FC,ELINKS,DAQ reset after power up", tasks)
  ->line("SCANCHARGE","Charge scan over all active channels", tasks)
  ->line("DELAYSCAN","Charge injection delay scan", tasks )
  ->line("PHASESCAN", "Scan phases", tasks)
  ->line("BEAMPREP", "Run settings and optional configuration for taking beamdata", tasks)
  ->line("CALIBRUN", "Produce the calibration scans", tasks)
  ->line("TUNE_TOT", "Tune TOT globally and per-channel", tasks)
  ->line("PEDESTAL_READ", "Take a single pedestal event, and dump details to stdout", tasks)
  ->line("CHARGE_READ", "Take a single calib_pulse event, and dump details to stdout", tasks)
  ->line("ALIGN_PREAMP",
         "Attempt to automatically tune REF_DAC_INV to hit a particular pedestal +- tolerance",
         tasks)
  ->line("DACB","Attempt to manually tune the channel-wise DACB parameter",tasks)
;
}

