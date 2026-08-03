#include "timein.h"

#include "pflib/TRIG.h"
#include "pflib/packing/Hex.h"
#include "pflib/packing/MultiSampleECONDEventPacket.h"
#include "pflib/packing/SingleECONTCaptureFrame.h"
#include "pflib/packing/TrigAlgoOutput.h"
#include "pflib/utility/string_format.h"

using pflib::packing::SingleECONTCaptureFrame;
using pflib::packing::TrigAlgoOutput;

void TimeInSettings::init(Target* tgt) {
  /**
   * if the values are zero, copy them from the chips
   */
  if (charge_to_l1a == 0) {
    bool _enable;
    tgt->fc().fc_get_setup_calib(charge_to_l1a, _enable);
  }
  if (l1offset == 0) {
    // just using one ROC for now
    auto dh_page = tgt->roc(pftool::state.iroc).getParameters("DIGITALHALF_0");
    l1offset = dh_page.at("L1OFFSET");
  }
  if (pipeline == 0) {
    int _econid, _samples, _pre;
    auto trig = tgt->trig();
    if (!trig) return;
    trig->get_daq_setup(pipeline, _econid, _samples, _pre);
  }
}

void TimeInSettings::apply(Target* tgt) const {
  std::map<std::string, std::map<std::string, uint64_t>> roc_settings;
  roc_settings["DIGITALHALF_0"]["L1OFFSET"] = this->l1offset;
  roc_settings["DIGITALHALF_1"]["L1OFFSET"] = this->l1offset;
  for (int iroc : tgt->roc_ids()) {
    tgt->roc(iroc).applyParameters(roc_settings);
  }

  bool _enable;
  int _ctl;
  tgt->fc().fc_get_setup_calib(_ctl, _enable);
  tgt->fc().fc_setup_calib(this->charge_to_l1a, _enable);

  int _pipeline, samples_per_l1a, presamples, econid;
  auto trig = tgt->trig();
  if (!trig) return;
  trig->get_daq_setup(_pipeline, econid, samples_per_l1a, presamples);
  trig->setup_daq(this->pipeline, econid, samples_per_l1a, presamples);
}

void TimeInSettings::update(int new_l1offset, int new_pipeline,
                            int new_charge_to_l1a) {
  if (new_charge_to_l1a < 0) {
    /**
     * if charge_to_l1a is not updated, we shift
     * it by the difference between the new and old l1offset
     */
    this->charge_to_l1a += (new_l1offset - this->l1offset);
  } else {
    this->charge_to_l1a = new_charge_to_l1a;
  }
  this->l1offset = new_l1offset;
  this->pipeline = new_pipeline;
}

TimeInSettings TimeInSettings::last{};

ENABLE_LOGGING();

void timein(Target* tgt) {
  /**
   * TRIG.TIMEIN
   */
  pflib::TRIG* trig = tgt->trig();
  if (trig == 0) return;

  TimeInSettings::last.init(tgt);

  /**
   * This command attempts to deduce the capture delay for the trigger
   * links by taking two runs after setting some parameters on the chip.
   *
   * Assuming the pedestal values on the chip are all ~200 (as is the case
   * at UMN), setting the CH_XX.ADC_PEDESTAL and DIGITALHALF_X.ADC_TH to
   * their maxima (255 and 31 respectively) forces the trigger sums to be
   * zero for pedestals.
   */

  pflib_log(info) << "setting up parameters for trigger link testing";

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

  /**
   * We inject a pulse into Channel 0 of ROC0 on the HcalBackplane
   * which comes out of ROC0 inside TC0_0 which goes into ECON-T1
   * DIN6 which then is summed into STC6 (I think)
   */
  static const int iroc_oi = 0, ch_oi = 0, stc_oi = 6;
  auto roc_inject =
      tgt->roc(iroc_oi).testParameters().add("CH_0", "LOWRANGE", 1).apply();

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  do {
    bool enable_l1a_follow;
    int og_charge_to_l1a;
    tgt->fc().fc_get_setup_calib(og_charge_to_l1a, enable_l1a_follow);

    int charge_to_l1a =
        pftool::readline_int("Calibration to L1A offset?", og_charge_to_l1a);
    tgt->fc().fc_setup_calib(charge_to_l1a, enable_l1a_follow);

    auto dh_page = tgt->roc(iroc_oi).getParameters("DIGITALHALF_0");
    int og_l1offset = dh_page.at("L1OFFSET");
    int l1offset = pftool::readline_int("L1Offset on HGCROC?", og_l1offset);
    auto test_l1offset_handle = tgt->roc(iroc_oi)
                                    .testParameters()
                                    .add("DIGITALHALF_0", "L1OFFSET", l1offset)
                                    .add("DIGITALHALF_1", "L1OFFSET", l1offset)
                                    .apply();

    int og_pipeline, og_samples_per_l1a, og_presamples, econid;
    trig->get_daq_setup(og_pipeline, econid, og_samples_per_l1a, og_presamples);
    int pipeline{og_pipeline}, samples_per_l1a{og_samples_per_l1a},
        presamples{og_presamples};
    pipeline = pftool::readline_int("pipeline: ", pipeline);
    samples_per_l1a =
        pftool::readline_int("samples_per_l1a: ", samples_per_l1a);
    presamples = pftool::readline_int("presamples: ", presamples);
    trig->setup_daq(pipeline, econid, samples_per_l1a, presamples);

    TimeInSettings::last.update(l1offset, pipeline, charge_to_l1a);

    pflib_log(info)
        << "pedestal runs to confirm alignment and trigger-sum suppression";
    tgt->fc().sendROR();
    usleep(10000);  // one 100Hz cycle later

    // capture data from this event
    std::vector<uint32_t> trg_pedestal_event = trig->read_event();
    std::vector<uint32_t> pedestal_algo_output_raw = trig->read_algo_output();
    std::vector<uint32_t> daq_pedestal_event = tgt->read_event();

    // decode captured data
    std::vector<SingleECONTCaptureFrame> trg_pedestals =
        decode_multi_sample<SingleECONTCaptureFrame>(trig->get_l1a_per_ror(),
                                                     trg_pedestal_event);

    std::vector<TrigAlgoOutput> pedestal_algo_output =
        decode_multi_sample<TrigAlgoOutput>(trig->get_l1a_per_ror(),
                                            pedestal_algo_output_raw);

    pflib::packing::MultiSampleECONDEventPacket daq_pedestals(2);
    daq_pedestals.from(daq_pedestal_event);

    pflib_log(info) << "charge injection run to see non-zero trigger sums in "
                       "specific places";
    tgt->fc().chargepulse();
    usleep(10000);  // one 100Hz cycle later

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

    pflib_log(debug) << "reset charge_to_l1a back to " << og_charge_to_l1a;
    tgt->fc().fc_setup_calib(og_charge_to_l1a, enable_l1a_follow);

    pflib_log(debug)
        << "reset capture pipeline and n_samples back to original settings";
    pflib_log(debug) << "original pipeline = " << og_pipeline
                     << " samples_per_l1a = " << og_samples_per_l1a
                     << " presamples = " << og_presamples;
    trig->setup_daq(og_pipeline, econid, og_samples_per_l1a, og_presamples);

    pflib_log(info) << "analyze words readout from links";
    pflib_log(info) << "with charge_to_l1a = " << charge_to_l1a
                    << " roc.l1offset = " << l1offset;
    pflib_log(info) << "with pipeline = " << pipeline
                    << " samples_per_l1a = " << samples_per_l1a
                    << " presamples = " << presamples;
    printf("DAQ Data\n");
    printf("     pedestal ->  charge\n");
    printf(" i:  t-1   t  ->  t-1   t \n");
    auto [i_erx, i_ch] = tgt->getRocErxMapping().toErxChannel(iroc_oi, ch_oi);
    for (int i_sample{0}; i_sample < daq_pedestals.samples.size(); i_sample++) {
      printf("%c%d: %4d %4d -> %4d %4d\n",
             (i_sample == tgt->daq().soi()) ? '*' : ' ', i_sample,
             daq_pedestals.samples.at(i_sample).channel(i_erx, i_ch).adc_tm1(),
             daq_pedestals.samples.at(i_sample).channel(i_erx, i_ch).adc(),
             daq_charge.samples.at(i_sample).channel(i_erx, i_ch).adc_tm1(),
             daq_charge.samples.at(i_sample).channel(i_erx, i_ch).adc());
    }

    printf("TRG Data\n");
    printf(" i:  ped -> chrg\n");
    for (int i_l1a{0}; i_l1a < trg_pedestals.size(); i_l1a++) {
      for (int i_sample{0}; i_sample < trg_pedestals[i_l1a].n_samples();
           i_sample++) {
        int pedestal{trg_pedestals[i_l1a].stc_sum(stc_oi, i_sample)},
            charge{trg_charge[i_l1a].stc_sum(stc_oi, i_sample)};
        printf("%c%d: %4d -> %4d (0x%03x)\n",
               (i_l1a + i_sample == tgt->daq().soi()) ? '*' : ' ',
               i_l1a + i_sample, pedestal, charge,
               trg_charge[i_l1a].encoded_stc_sum(stc_oi, i_sample));
      }
    }

    printf("ALGO Output\n");
    printf(" i: charge output\n");
    for (int i_l1a{0}; i_l1a < pedestal_algo_output.size(); i_l1a++) {
      for (int i_sample{0}; i_sample < pedestal_algo_output[i_l1a].n_samples();
           i_sample++) {
        if (i_l1a + i_sample == tgt->daq().soi()) {
          std::cout << '*';
        } else {
          std::cout << ' ';
        }
        std::cout << i_l1a + i_sample << ": "
                  << charge_algo_output[i_l1a].sample(i_sample) << std::endl;
      }
    }
  } while (pftool::readline_bool(
      "Want to try another set of timing parameters?", false));
}
