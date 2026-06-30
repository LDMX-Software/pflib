#include "examine_phase.h"

#include <algorithm>
#include <cmath>

#include "../daq_run.h"
#include "pflib/utility/efficiency.h"
#include "pflib/utility/string_format.h"
#include "pflib/utility/mean.h"

void scan_phase_strobe(Target* tgt, pflib::ROC& roc, int i_roc, DecodeAndBuffer& buffer, int nevents) {
  static auto the_log_{::pflib::logging::get("scan_phase_strobe")};
  //scan entire peak
  int nr_bx = 5;
  int central_charge_to_l1a = tgt->fc().fc_get_setup_calib();
  if (central_charge_to_l1a > 3) {
    tgt->fc().fc_setup_calib(central_charge_to_l1a - 2);
  }
  else if (central_charge_to_l1a == 3) {
    tgt->fc().fc_setup_calib(central_charge_to_l1a - 1);
  }
  pflib::DAQ& daq = tgt->daq();
  int initial_nr_bx = daq.samples_per_ror();
  daq.setup(daq.econid(), nr_bx, daq.soi());
  tgt->fc().setL1AperROR(nr_bx);


  pflib_log(info) << "Scanning phase_strobe";
  for (int phase_strobe = 0; phase_strobe < 16; phase_strobe ++){
    pflib_log(info) << "Phase_strobe: " << phase_strobe;
    auto test_param = roc.testParameters()
                         .add("REFERENCEVOLTAGE_0", "INTCTEST", 1)
                         .add("REFERENCEVOLTAGE_0", "CHOICE_CINJ", 1)
                         .add("CH_17", "HIGHRANGE", 1)
                         .add("REFERENCEVOLTAGE_0", "TOA_VREF", 0)
                         .add("REFERENCEVOLTAGE_0", "CALIB", 400)
                         .add("TOP", "PHASE_STROBE", phase_strobe)
                         .apply();

      std::map<int, std::vector<int>> adcs;
      usleep(10);
      daq_run(tgt, "CHARGE", buffer, nevents, pftool::state.daq_rate);
      auto data = buffer.get_buffer();
      auto mapping = tgt->getRocErxMapping();
      auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, 17);
      for (std::size_t i{0}; i < data.size(); i++) {
        for (int j = 0; j < nr_bx; j++) {
          adcs[j].push_back(
               data[i].samples.at(j).channel(i_erx, i_ch).adc());
        }
      }
      for (int j = 0; j < nr_bx; j++) {
        int mean_adc = pflib::utility::mean(adcs[j]);
        pflib_log(info) << "phase_strobe: " << phase_strobe << " adc: " << mean_adc;
      }


  }
  tgt->fc().fc_setup_calib(central_charge_to_l1a);
  daq.setup(daq.econid(), nr_bx, daq.soi());
  tgt->fc().setL1AperROR(initial_nr_bx);
}

void scan_phase_ck(Target* tgt, DecodeAndBuffer& buffer, int nevents) {
  static auto the_log_{::pflib::logging::get("scan_phase_ck")};
  // Loop over phases and do pedestals

  for (int phase_ck = 0; phase_ck < 16; phase_ck++) {

    pflib_log(info) << "Scanning phase_ck = " << phase_ck;

    std::map<std::string, std::map<std::string, uint64_t>>
        parameters;
    parameters["TOP"]["PHASE_CK"] = phase_ck;

    auto test_params = tgt->tempApplyAllROCs(parameters);
    usleep(10);

    daq_run(tgt, "PEDESTAL", buffer, nevents, pftool::state.daq_rate);
    auto data = buffer.get_buffer();
    std::map<int, std::vector<int>> adcs;


    for (int i_roc : tgt->roc_ids()) {
      const pflib::packing::SingleECONDRocErxMapping& mapping =
          tgt->getRocErxMapping();
      for (int ch = 0; ch < 72; ch++) {
        auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, ch);
        for (std::size_t i{0}; i < data.size(); i++) {
          adcs[i_roc].push_back(data[i].soi().channel(i_erx, i_ch).adc());
        }
      }
      pflib_log(info) << "phase_ck: " << phase_ck << " adc: " << pflib::utility::mean(adcs[i_roc]);
    }
  }
}

int peak_bx(Target* tgt, pflib::ROC& roc, int i_roc, DecodeAndBuffer& buffer, int nevents) {
  static auto the_log_{::pflib::logging::get("peak_bx")};
  //find bx of peak
  bool keep_going = true;
  int bx_calib = 1000;
  while (keep_going) {
    for (int bx = 1; bx <= 100; bx++) {
      std::vector<int> adcs;
      //set parameters
      auto test_param = roc.testParameters()
                            .add("REFERENCEVOLTAGE_0", "INTCTEST", 1)
                            .add("REFERENCEVOLTAGE_0", "CHOICE_CINJ", 1)
                            .add("CH_17", "HIGHRANGE", 1)
                            .add("REFERENCEVOLTAGE_0", "TOA_VREF", 0)
                            .add("REFERENCEVOLTAGE_0", "CALIB", bx_calib)
                            .add("TOP", "PHASE_STROBE", 0)
                            .apply();
      tgt->fc().fc_setup_calib(bx);
      pflib_log(info) << "Testing bx: " << tgt->fc().fc_get_setup_calib();
      usleep(10);

      //do a run and collect data
      daq_run(tgt, "CHARGE", buffer, nevents, pftool::state.daq_rate);
      auto data = buffer.get_buffer();
      auto mapping = tgt->getRocErxMapping();
      auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, 17);
      for (std::size_t i{0}; i < data.size(); i++) {
        adcs.push_back(
          data[i].soi().channel(i_erx, i_ch).adc());
      }

      //determine if we have reached the peak
      int max_adc = *std::max_element(adcs.begin(), adcs.end());
      pflib_log(info) << "max adc: " << max_adc;
      if (max_adc == 1023) {
        keep_going = false;
        break;
      }

    }

    if (keep_going == true && bx_calib < 4000) {
      bx_calib += 100;
    }
    else {
    break;
    }

  }
  auto central_charge_to_l1a = tgt->fc().fc_get_setup_calib();
  pflib_log(info) << "Final bx: " << central_charge_to_l1a;
  return central_charge_to_l1a;
}

void examine_phase(Target* tgt) {
  static auto the_log_{::pflib::logging::get("examine_phase")};
  static const std::size_t nevents = pftool::readline_int("How many events per time point? ", 100);
  pflib::ROC roc{tgt->roc(pftool::state.iroc)};
  int i_roc = pftool::state.iroc;
  DecodeAndBuffer buffer{nevents, 2 * tgt->nrocs()};
  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  //find bx of charge pulse
  int central_charge_to_l1a = peak_bx(tgt, roc, i_roc, buffer, nevents);

  //scan phase_ck
  scan_phase_ck(tgt, buffer, nevents);

  //scan phase_strobe
  scan_phase_strobe(tgt, roc, i_roc, buffer, nevents);

  pflib_log(info) << "bx is set to " << tgt->fc().fc_get_setup_calib();
}
