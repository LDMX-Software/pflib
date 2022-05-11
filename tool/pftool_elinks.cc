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

void auto_align(pflib::PolarfireTarget* pft) {

  static float threshold = 0.2;
  threshold = BaseMenu::readline_float("What should be our threshold value?");
  static int nevents = 100;
  nevents = BaseMenu::readline_int("How many events for the header checks?", nevents);

  static int delay_step = 10;
  delay_step = BaseMenu::readline_int("Delay step: ",delay_step);
  auto results {header_check(pft, nevents)};
  pflib::Elinks& elinks=pft->hcal.elinks();
  while(true) {
    for (int i {0}; i < 100; ++i) {
      pft->hcal.resyncLoadROC(-1);
      for (int j {0}; j < 5; ++j) {
        elinks.resetHard();
        align_elinks(pft, elinks, delay_step, false);
        results = header_check(pft, nevents, false);
        if (results.is_acceptable(threshold)) {
          return;
        }
      }
    }

    if (BaseMenu::readline_bool("Try a new threshold?", false)) {
      threshold = BaseMenu::readline_float("What should be our threshold value?");
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
    setup_dma(pft, false);
  }

    int nsamples=1;
    {
      bool multi;
      int nextra;
      pft->hcal.fc().getMultisampleSetup(multi,nextra);
      if (multi) nsamples=nextra+1;
    }
    constexpr const int num_links {8};
    const std::vector<int> activeLinkNumbers = getActiveLinkNumbers(pft);
    HeaderCheckResults results {activeLinkNumbers};

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

void align_elinks(PolarfireTarget* pft, pflib::Elinks& elinks, const int delay_step, bool verbose)
{
  const int nevents=10;
  int nsamples=1;
  {
    bool multi;
    int nextra;
    pft->hcal.fc().getMultisampleSetup(multi,nextra);
    if (multi) nsamples=nextra+1;
  }


  const std::vector<int> activeLinkNumbers = getActiveLinkNumbers(pft);
  const std::size_t num_active_links {activeLinkNumbers.size()};
  std::vector<int> bitslip_candidate(num_active_links);
  std::vector<int> delay_candidate(num_active_links);
  std::vector<HeaderStatus> record_status(num_active_links);

  pft->prepareNewRun();
  constexpr const int bitslip_max = 8;
  constexpr const int bitslip_min = 0;

  for(int bitslip = bitslip_min; bitslip < bitslip_max; bitslip += 1){
    std::cout << "Scanning bitslip: " << bitslip << " of " << bitslip_max -1 << std::endl;
    for(int delay = 0; delay <= 128; delay += delay_step) {
      for(auto link : activeLinkNumbers){
        elinks.setDelay(link,delay);
        elinks.setBitslipAuto(link,false);
        elinks.setBitslip(link,bitslip);
      }
      HeaderCheckResults results{activeLinkNumbers};
      // int n_good_bxheaders[num_active_links] {};
      // int n_bad_bxheaders[num_active_links] {};
      // int n_good_idles[num_active_links] {};
      // int n_bad_idles[num_active_links] {};
      for (int ievt{0}; ievt < nevents; ievt++) {
        pft->backend->fc_sendL1A();
        std::vector<uint32_t> event_raw = pft->daqReadEvent();
        pflib::decoding::SuperPacket event{&(event_raw[0]), int(event_raw.size())};
        results.add_event(event, nsamples);
      }
      for(int i = 0; i < num_active_links; i++){
        if(n_good_idles[i] >= record_idle[i] && n_good_bxheaders[i] >= record_bx[i]){
          record_bx[i] = n_good_bxheaders[i];
          record_idle[i] = n_good_idles[i];
          bitslip_candidate[i] = bitslip;
          delay_candidate[i] = delay;
        }
      }

      /*
        for(int jlink = 0; jlink < 8; jlink++){
        std::cout << n_good_bxheaders[jlink] << ":" << n_good_idles[jlink] << "  ";
        }
        std::cout << std::endl;
      */
    }
  }
  if (verbose) {
    std::cout << "Candidates" << std::endl;
  }
  for(int i = 0; i < num_active_links; i++){

  if (verbose) {
    std::cout << "Link " << i << std::endl;
    std::cout << "Bitslip: " << bitslip_candidate[i] << "   Delay: " << delay_candidate[i] << std::endl;
  }


    elinks.setDelay(i,delay_candidate[i]);
    elinks.setBitslipAuto(i,false);
    elinks.setBitslip(i,bitslip_candidate[i]);

    float bg_bxheaders = 1.;
    int bad_bxheaders = nsamples*nevents-record_bx[i];
    if (record_bx[i]>0)
      bg_bxheaders = float(bad_bxheaders)/
        (record_bx[i]+bad_bxheaders);
    if (verbose) {
      std::cout << "Percentage of bad BX headers: " << bg_bxheaders*100. << std::endl;
    }


    if(bg_bxheaders > 0.1 && verbose){
      std::cout << "Consider a PLL hard_reset" << std::endl;
    }
    float bg_idles = 1.;
    int bad_idles = nsamples*nevents-record_idle[i];
    if (record_idle[i]>0)
      bg_idles = float(bad_idles)/
        (record_idle[i]+bad_idles);
    if (verbose) {
    std::cout << "Percentage of bad idles: " << bg_idles*100. << std::endl;
    if(bg_idles > 0.1){
      std::cout << "Consider a PLL hard_reset" << std::endl;
    }
    }
  }
}
void align_elinks(PolarfireTarget* pft, pflib::Elinks& elinks) {

  static int delay_step = 10;
  delay_step = BaseMenu::readline_int("Delay step: ",delay_step);
  align_elinks(pft, elinks, delay_step);

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
    std::cout << "Running header check..." << std::endl;
    header_check(pft, 100);
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
    std::cout << "Running elinks alignment" << std::endl;
    align_elinks(pft, elinks);
    std::cout << "Running header check..." << std::endl;
    header_check(pft, 250);
  }
  if (cmd=="SCAN") {
    ilink=BaseMenu::readline_int("Which elink? ",ilink);
    elinks.scanAlign(ilink);
  }
  if (cmd=="STATUS") {
    pft->elinkStatus(std::cout);
  }
}
