#include "self_trig.h"

#include "timein.h" // for TimeInSettings::last
#include "decode_multi_sample.h"

#include "pflib/TRIG.h"
#include "pflib/utility/string_format.h"
#include "pflib/packing/Hex.h"
#include "pflib/packing/MultiSampleECONDEventPacket.h"
#include "pflib/packing/SingleECONTCaptureFrame.h"
#include "pflib/packing/TrigAlgoOutput.h"

using pflib::packing::SingleECONTCaptureFrame;
using pflib::packing::TrigAlgoOutput;

ENABLE_LOGGING();

void self_trig(Target* tgt) {
  /**
   * TRIG.SELF_TRIG
   */
  pflib::TRIG* trig = tgt->trig();
  if (trig == 0) return;

  /**
   * Attempts to have trigger algo do the self-triggering by
   * disabling the following L1A after a charge injection
   * and enabling external L1As originating from our trigger
   * algorithm.
   */
  pflib_log(info) << "setting up parameters for self-trigger test";

  std::map<std::string, std::map<std::string, uint64_t>> roc_setup;
  for (int half{0}; half < 2; half++) {
    roc_setup[pflib::utility::string_format("HALFWISE_%d", half)]
             ["ADC_PEDESTAL"] = 255;
    roc_setup[pflib::utility::string_format("DIGITALHALF_%d", half)]["ADC_TH"] =
        31;
    auto refvol_page{
        pflib::utility::string_format("REFERENCEVOLTAGE_%d", half)};
    roc_setup[refvol_page]["CALIB"] = 3000;
    roc_setup[refvol_page]["INTCTEST"] = 1;
  }
  auto roc_test_lock = tgt->tempApplyAllROCs(roc_setup);

  static const int iroc_oi = 0, ch_oi = 0, stc_oi = 6;
  auto roc_inject =
      tgt->roc(iroc_oi).testParameters().add("CH_0", "LOWRANGE", 1).apply();

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  bool enable_l1a_follow;
  int charge_to_l1a;
  tgt->fc().fc_get_setup_calib(charge_to_l1a, enable_l1a_follow);
  pflib_log(info) << "charge_to_l1a = " << charge_to_l1a
                  << " enable_l1a_follow = " << enable_l1a_follow;
  bool l1aen, extl1a;
  tgt->fc().fc_enables_read(l1aen, extl1a);
  pflib_log(info) << "l1a_enabled = " << l1aen << " external_l1a = " << extl1a;
  pflib_log(info) << "self-trigger count: " << trig->get_self_trigger_count();
  pflib_log(info) << "event occupancy: " << tgt->daq().getEventOccupancy();

  pflib_log(info) << "disabling the L1A following the charge command";
  tgt->fc().fc_setup_calib(charge_to_l1a, false);

  pflib_log(info) << "enabling single-shot external L1A mode";
  bool og_single_shot = trig->get_enable_single_shot();
  trig->enable_single_shot(true);
  tgt->fc().fc_enables(true, true);

  pflib_log(info) << "self-trigger count: " << trig->get_self_trigger_count();
  pflib_log(info) << "event occupancy: " << tgt->daq().getEventOccupancy();

  int last_l1offset{TimeInSettings::last.l1offset},
      last_pipeline{TimeInSettings::last.pipeline};
  do {
    int l1offset = pftool::readline_int("L1Offset on HGCROC?", last_l1offset);
    auto test_l1offset_handle = tgt->roc(iroc_oi)
                                    .testParameters()
                                    .add("DIGITALHALF_0", "L1OFFSET", l1offset)
                                    .add("DIGITALHALF_1", "L1OFFSET", l1offset)
                                    .apply();

    int og_pipeline, og_samples_per_l1a, og_presamples, econid;
    trig->get_daq_setup(og_pipeline, econid, og_samples_per_l1a, og_presamples);
    int pipeline{og_pipeline}, samples_per_l1a{og_samples_per_l1a},
        presamples{og_presamples};
    pipeline = pftool::readline_int("pipeline: ", last_pipeline);
    trig->setup_daq(pipeline, econid, samples_per_l1a, presamples);

    pflib_log(info) << "self-trigger count: " << trig->get_self_trigger_count();
    pflib_log(info) << "event occupancy: " << tgt->daq().getEventOccupancy();
    pflib_log(info) << "reset single shot and then inject a charge pulse";
    trig->reset_single_shot();
    tgt->fc().chargepulse();
    int i100us{0};
    do {
      usleep(100);
      i100us++;
    } while (not trig->single_shot_fired() and i100us < 10);
    pflib_log(info) << "single shot fired: " << std::boolalpha
                    << trig->single_shot_fired();
    pflib_log(info) << "self-trigger count: " << trig->get_self_trigger_count();
    pflib_log(info) << "event occupancy: " << tgt->daq().getEventOccupancy();

    if (tgt->daq().getEventOccupancy() == 1) {
      // capture data output, using daq last to advance readout pointer
      std::vector<uint32_t> trg_charge_event = trig->read_event();
      std::vector<uint32_t> charge_algo_output_raw = trig->read_algo_output();
      std::vector<uint32_t> daq_charge_event = tgt->read_event();

      // decode after capturing all data so decoding errors don't cause
      // readout pointer misalignment
      std::vector<SingleECONTCaptureFrame> trg_charge =
          decode_multi_sample<SingleECONTCaptureFrame>(trig->get_l1a_per_ror(),
                                                       trg_charge_event);

      std::vector<TrigAlgoOutput> charge_algo_output =
          decode_multi_sample<TrigAlgoOutput>(trig->get_l1a_per_ror(),
                                              charge_algo_output_raw);

      pflib::packing::MultiSampleECONDEventPacket daq_charge(2);
      daq_charge.from(daq_charge_event);

      printf("DAQ Data\n");
      printf(" i:  t-1   t \n");
      auto [i_erx, i_ch] = tgt->getRocErxMapping().toErxChannel(iroc_oi, ch_oi);
      for (int i_sample{0}; i_sample < daq_charge.samples.size(); i_sample++) {
        printf("%2d: %4d %4d\n", i_sample,
               daq_charge.samples.at(i_sample).channel(i_erx, i_ch).adc_tm1(),
               daq_charge.samples.at(i_sample).channel(i_erx, i_ch).adc());
      }

      printf("TRG Data\n");
      printf(" i: stc6\n");
      for (int i_l1a{0}; i_l1a < trg_charge.size(); i_l1a++) {
        for (int i_sample{0}; i_sample < trg_charge[i_l1a].n_samples();
             i_sample++) {
          int charge{trg_charge[i_l1a].stc_sum(stc_oi, i_sample)};
          printf("%2d: %4d\n", i_l1a + i_sample, charge);
        }
      }

      printf("ALGO Output\n");
      printf(" i: charge output\n");
      for (int i_l1a{0}; i_l1a < charge_algo_output.size(); i_l1a++) {
        for (int i_sample{0}; i_sample < charge_algo_output[i_l1a].n_samples();
             i_sample++) {
          std::cout << std::setw(2) << i_l1a + i_sample
                    << ": " << charge_algo_output[i_l1a].sample(i_sample)
                    << std::endl;
        }
      }
    }
    trig->setup_daq(og_pipeline, econid, og_samples_per_l1a, og_presamples);
  } while (pftool::readline_bool(
      "Want to try another set of timing parameters?", false));

  // shift the charge-to-l1a by the same amount we shifted the
  // l1offset so that charge injection is still in time
  int delta_bx = (last_l1offset - TimeInSettings::last.l1offset);
  TimeInSettings::last.charge_to_l1a += delta_bx;
  TimeInSettings::last.l1offset = last_l1offset;
  TimeInSettings::last.pipeline = last_pipeline;

  pflib_log(info) << "disabling single-shot external L1A";
  tgt->fc().fc_enables(l1aen, extl1a);
  trig->enable_single_shot(og_single_shot);

  tgt->fc().fc_setup_calib(charge_to_l1a, enable_l1a_follow);
}
