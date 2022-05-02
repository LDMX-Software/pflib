
#include "pftool_fastcontrol.h"
using pflib::PolarfireTarget;

void fc_calib(PolarfireTarget* pft,
              const int len, const int offset) {
    pft->backend->fc_setup_calib(len,offset);
}
void fc_calib(PolarfireTarget* pft) {
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
    fc_calib(pft, len, offset);
}


void multisample_setup(pflib::PolarfireTarget* pft,
                       const bool enable,
                       const int nextra)
{

    pft->hcal.fc().setupMultisample(enable,nextra);
    // also, DMA needs to know
#ifdef PFTOOL_ROGUE
    auto rwbi=dynamic_cast<pflib::rogue::RogueWishboneInterface*>(pft->wb);
    if (rwbi) {
      // Note: We read more information than we actually need here which is why
      // we have all of these extra variables. We only actually need the FPGA_ID.
      bool enabled;
      uint8_t samples_per_event, fpgaid_i;
      rwbi->daq_get_dma_setup(fpgaid_i,samples_per_event, enabled);
      samples_per_event=nextra+1;
      rwbi->daq_dma_setup(fpgaid_i,samples_per_event);
    }
#endif
}

void multisample_setup(pflib::PolarfireTarget* pft)
{
    bool multi;
    int nextra;
    pft->hcal.fc().getMultisampleSetup(multi,nextra);
    multi=BaseMenu::readline_bool("Enable multisample readout? ",multi);
    if (multi) {
      nextra=BaseMenu::readline_int("Extra samples (total is +1 from this number) : ",nextra);
    }
    multisample_setup(pft, multi, nextra);

}

void fc( const std::string& cmd, PolarfireTarget* pft ) {
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
      veto_setup(pft);
  }
  if (cmd=="COUNTER_RESET") {
    pft->hcal.fc().resetCounters();
    do_status=true;
  }
  if (cmd=="FC_RESET") {
    pft->hcal.fc().resetTransmitter();
  }
  if (cmd=="CALIB") {
    fc_calib(pft);
  }
  if (cmd=="MULTISAMPLE") {
    multisample_setup(pft);
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
    fc_enables(pft);
  }
}


void veto_setup(pflib::PolarfireTarget* pft,
                bool veto_daq_busy,
                bool veto_l1_occ,
                bool ask)
{

  const bool veto_daq_busy_before_read{veto_daq_busy};
  const bool veto_l1_occ_before_read{veto_l1_occ};
    int level_busy, level_ok;
    pft->backend->fc_veto_setup_read(veto_daq_busy, veto_l1_occ,level_busy, level_ok);
    std::cout << "Veto setup before: " << std::boolalpha << veto_daq_busy << ", " << veto_l1_occ << "\n";
    if (ask) {
      veto_daq_busy=BaseMenu::readline_bool("Veto L1A on DAQ busy? ",veto_daq_busy);
      veto_l1_occ=BaseMenu::readline_bool("Veto L1A on L1 occupancy? ",veto_l1_occ);
    } else {
      veto_daq_busy = veto_daq_busy_before_read;
      veto_l1_occ = veto_l1_occ_before_read;
    }
    std::cout << "Veto setup after: " << std::boolalpha << veto_daq_busy << ", " << veto_l1_occ << "\n";
    std::cout << "Levels: " << level_busy << " " << level_ok << "\n";
    pft->backend->fc_veto_setup(veto_daq_busy, veto_l1_occ,level_busy, level_ok);
    if (veto_l1_occ) {
        printf("\n  Occupancy Veto Thresholds -- OK->BUSY at %d, BUSY->OK at %d\n",level_busy,level_ok);
    }
}


void fc_enables(pflib::PolarfireTarget* pft)
{
    bool ext_l1a, ext_spill, timer_l1a;

    pft->backend->fc_enables_read(ext_l1a, ext_spill, timer_l1a);
    std::cout << std::boolalpha << "Enables before: " << ext_l1a << ", " << ext_spill << ", " << timer_l1a << "\n";
    ext_l1a=BaseMenu::readline_bool("Enable external L1A? ",ext_l1a);
    ext_spill=BaseMenu::readline_bool("Enable external spill? ",ext_spill);
    timer_l1a=BaseMenu::readline_bool("Enable timer L1A? ",timer_l1a);
    std::cout << std::boolalpha << "Enables after: " << ext_l1a << ", " << ext_spill << ", " << timer_l1a << "\n";
    fc_enables(pft, ext_l1a, ext_spill, timer_l1a);
}
void fc_enables(pflib::PolarfireTarget* pft,
                const bool external_l1a,
                const bool external_spill,
                const bool timer_l1a)
{
  std::cout << "Setting fc_enables: " << std::boolalpha << external_l1a << ", " << external_spill << ", " << timer_l1a <<'\n';
    pft->backend->fc_enables(external_l1a, external_spill, timer_l1a);
}
