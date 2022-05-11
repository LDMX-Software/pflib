#include "pftool_elinks.h"
using pflib::PolarfireTarget;


std::vector<int> getActiveLinkNumbers(pflib::PolarfireTarget* pft)
{

    constexpr const int max_links = 8;
    std::vector<int> activeLinks{};
    const pflib::Elinks& elinks {pft->hcal.elinks()};
    for (int i {0}; i < max_links; ++i) {
        if(elinks.isActive(i)) {
            activeLinks.push_back(i);
        }
    }
    return activeLinks;
}

std::vector<float> link_thresholds(pflib::PolarfireTarget* pft) {
  const auto activeLinkNumbers {getActiveLinkNumbers(pft)};
  if (BaseMenu::readline_bool("Set individual thresholds for each active link? ", false)) {
    std::vector<float> thresholds{};
    for (auto link : activeLinkNumbers) {
      float threshold = BaseMenu::readline_float("Threshold value for link "  + std::to_string(link) + ": ");
      thresholds.push_back(threshold);
    }
    return thresholds;

  } else {
    float threshold = BaseMenu::readline_float("What should be our threshold value? ");
    std::vector<float> thresholds(threshold, activeLinkNumbers.size());
    return thresholds;
  }
}

void auto_align(pflib::PolarfireTarget* pft) {

  static int nevents = 100;
  nevents = BaseMenu::readline_int("How many events for the header checks?", nevents);
  auto thresholds = link_thresholds(pft);

  static int delay_steps = 10;
  delay_steps = BaseMenu::readline_int("Number of delay steps: ",delay_steps);
  auto results {header_check(pft, nevents)};
  pflib::Elinks& elinks=pft->hcal.elinks();
  while(true) {
    for (int i {0}; i < 100; ++i) {
      std::cout << "Running ResyncLoad" << std::endl;
      pft->hcal.resyncLoadROC(-1);
      for (int j {0}; j < 5; ++j) {
        elinks.resetHard();
        align_elinks(pft, elinks, delay_steps, false);
        results = header_check(pft, nevents, false);
        if (results.is_acceptable(thresholds)) {
          std::cout << "Success!" << std::endl;
          std::cout << "Redoing bitslip/delay scan with maximum granularity" << std::endl;
          align_elinks(pft, elinks, 128, false);
          return;
        }
      }
    }

    if (BaseMenu::readline_bool("Try a new threshold?", false)) {
      thresholds = link_thresholds(pft);
    } else if (BaseMenu::readline_bool("Give up?", false)) {
      return;
    }
  }

}

HeaderCheckResults header_check(PolarfireTarget* pft, const int nevents,
                                const bool verbose)
{

  if (verbose) {
    std::cout << "Disabling DMA...\n";
  }
  setup_dma(pft, false);

  int nsamples=1;
  {
    bool multi;
    int nextra;
    pft->hcal.fc().getMultisampleSetup(multi,nextra);
    if (multi) nsamples=nextra+1;
  }
  const std::vector<int> activeLinkNumbers = getActiveLinkNumbers(pft);
  HeaderCheckResults results {activeLinkNumbers};
  results.verbose = verbose;

  pft->prepareNewRun();
  for (int ievt{0}; ievt < nevents; ievt++) {
    pft->backend->fc_sendL1A();
    std::vector<uint32_t> event_raw = pft->daqReadEvent();
    pflib::decoding::SuperPacket event{&(event_raw[0]), int(event_raw.size())};
    results.add_event(event, nsamples);
  }

  if (verbose) {
    results.report();

  }
  return results;
}

void align_elinks(PolarfireTarget* pft, pflib::Elinks& elinks, const int delay_steps, bool verbose)
{

  const std::vector<int> activeLinkNumbers = getActiveLinkNumbers(pft);
  const std::size_t num_active_links {activeLinkNumbers.size()};
  std::vector<int> bitslip_candidate(num_active_links);
  std::vector<int> delay_candidate(num_active_links);
  std::vector<HeaderStatus> record_status(num_active_links);

  constexpr const int bitslip_max = 8;
  constexpr const int bitslip_min = 0;
  constexpr const int delay_min = 0;
  constexpr const int delay_range_max = 128;

  const int delay_max = std::min(delay_steps, delay_max);
  constexpr const int nevents {10};

  for(int bitslip = bitslip_min; bitslip < bitslip_max; bitslip ++){
    std::cout << "Scanning bitslip: " << bitslip << " of " << bitslip_max -1 << std::endl;
    for(int delay = delay_min; delay < delay_max; delay++) {
      for(auto link : activeLinkNumbers){
        elinks.setDelay(link,delay);
        elinks.setBitslip(link,bitslip);
      }
      auto results {header_check(pft, nevents, false)};
      for(std::size_t  link_index{0}; link_index < num_active_links; ++link_index) {
        const auto& status = results.res[link_index];
        if (status.n_good_idles >= record_status[link_index].n_good_idles &&
            status.n_good_bxheaders >= record_status[link_index].n_good_bxheaders) {
          record_status[link_index]  = status;
          bitslip_candidate[link_index] = bitslip;
          delay_candidate[link_index] = delay;
        }
      }
    }
  }
  if (verbose) {
    std::cout << "Candidates" << std::endl;
  }
  for(int link_index = 0; link_index < num_active_links; link_index++){

    const auto record = record_status[link_index];
    const int link = record.link;
    const int bitslip = bitslip_candidate[link_index];
    const int delay = delay_candidate[link_index];
    if (verbose) {
      std::cout << "Link " << link << std::endl;
      std::cout << "Bitslip: " << bitslip << "   Delay: " << delay << std::endl;
    }


    elinks.setDelay(link, delay);
    elinks.setBitslip(link,bitslip);


    if (verbose) {
      std::cout << "Percent of bad BX headers: " << record.percent_bad_headers() * 100. << std::endl;
      std::cout << "Percent of bad idles: " << record.percent_bad_idles() * 100. << std::endl;
      if (record.percent_bad_headers() > 0.1 || record.percent_bad_idles() > 0.1) {
        std::cout << "Consider a PLL hard_reset" << std::endl;
      }
    }
  }
}
void align_elinks(PolarfireTarget* pft, pflib::Elinks& elinks) {

  static int delay_steps = 10;
  delay_steps = BaseMenu::readline_int("Number of delay steps: ",delay_steps);
  align_elinks(pft, elinks, delay_steps);

}

void elinks( const std::string& cmd, PolarfireTarget* pft )
{
  pflib::Elinks& elinks=pft->hcal.elinks();
  static int ilink=0,nevents{100};
  static int min_delay{0}, max_delay{128};
  if (cmd == "AUTOALIGN") {
    if (BaseMenu::readline_bool("Are you sure you want to try to change the ELINK alignment?", false)) {
      auto_align(pft);
    }

    return;
  }
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
  if (cmd == "ALIGN") {
    align_elinks(pft, elinks);
    return;
  }
  if (cmd=="HEADER_CHECK") {
    nevents=BaseMenu::readline_int("Num events? ",nevents);
    header_check(pft, nevents);
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
