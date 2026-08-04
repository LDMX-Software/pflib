#include "watch_run.h"

#include "decode_multi_sample.h"
#include "pflib/TRIG.h"
#include "pflib/packing/Hex.h"
#include "pflib/packing/MultiSampleECONDEventPacket.h"
#include "pflib/packing/SingleECONTCaptureFrame.h"
#include "pflib/packing/TrigAlgoOutput.h"
#include "pflib/utility/string_format.h"
using pflib::packing::MultiSampleECONDEventPacket;
using pflib::packing::SingleECONTCaptureFrame;

ENABLE_LOGGING();

void watch_run(pflib::Target* tgt) {
  auto trig = tgt->trig();
  if (!trig) return;
  /**
   * TRIG.WATCH_RUN
   *
   * watch the self trigger and write out the
   * data that it collects
   */

  int n_events = pftool::readline_int("Number of events to wait for?", 100);
  auto path{pftool::readline_path("watch-run", ".csv")};
  std::ofstream file{path};
  if (not file.is_open()) {
    pflib_log(fatal) << "unabel to open " << path;
    return;
  }

  file << "i_event,i_sample";
  // TODO: choose channels to write out
  for (int i_ch{0}; i_ch < 8; i_ch++) {
    file << ",ch_" << i_ch << ".Tp"
         << ",ch_" << i_ch << ".Tc"
         << ",ch_" << i_ch << ".adc_tm1"
         << ",ch_" << i_ch << ".adc"
         << ",ch_" << i_ch << ".toa"
         << ",ch_" << i_ch << ".tot";
  }
  file << ",stc6\n";

  bool l1aen, extl1a;
  tgt->fc().fc_enables_read(l1aen, extl1a);
  bool og_single_shot = trig->get_enable_single_shot();
  trig->enable_single_shot(true);
  tgt->fc().fc_enables(true, true);

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  int self_trigger_count = trig->get_self_trigger_count();
  for (int i_event{0}; i_event < n_events; i_event++) {
    if (i_event % 100 == 0 and i_event > 99) {
      // status on every 100 events after the first 100
      pflib_log(info) << i_event << " events collected";
    }

    trig->reset_single_shot();
    int i100us{0};
    do {
      usleep(100);
      i100us++;
    } while (not trig->single_shot_fired() and i100us < 10000);

    if (not trig->single_shot_fired()) {
      pflib_log(warn)
          << "waiting for 1s and did not see a self-trigger, skipping event "
          << i_event;
      continue;
    }

    // capture data output, using daq last to advance readout pointer
    std::vector<uint32_t> trg_charge_event = trig->read_event();
    std::vector<uint32_t> charge_algo_output_raw = trig->read_algo_output();
    std::vector<uint32_t> daq_charge_event = tgt->read_event();

    // decode after capturing all data so decoding errors don't cause
    // readout pointer misalignment
    std::vector<SingleECONTCaptureFrame> trg_charge =
        decode_multi_sample<SingleECONTCaptureFrame>(trig->get_l1a_per_ror(),
                                                     trg_charge_event);

    /*
    std::vector<TrigAlgoOutput> charge_algo_output =
        decode_multi_sample<TrigAlgoOutput>(trig->get_l1a_per_ror(),
                                            charge_algo_output_raw);
    */

    pflib::packing::MultiSampleECONDEventPacket daq_charge(2);
    daq_charge.from(daq_charge_event);

    // serialize
    for (int i_sample{0}; i_sample < trig->get_l1a_per_ror(); i_sample++) {
      file << i_event << ',' << i_sample;
      for (int ch{0}; ch < 8; ch++) {
        auto sample{daq_charge.samples.at(i_sample).channel(
            1 /* should use mapping! */, ch)};
        file << ',' << sample.Tp() << ',' << sample.Tc() << ','
             << sample.adc_tm1() << ',' << sample.adc() << ',' << sample.toa()
             << ',' << sample.tot();
      }
      file << ',' << trg_charge[i_sample].stc_sum(6, 0) << '\n';
    }

    int new_self_trigger_count = trig->get_self_trigger_count();
    if (new_self_trigger_count != self_trigger_count+1) {
      // self trigger counter is 16bits and so we may have wrapped around
      // if its getting spammed
      int diff{0};
      if (new_self_trigger_count < self_trigger_count) {
        // wrap around happend
        diff = (0xffff - self_trigger_count) + new_self_trigger_count;
      } else {
        // no wrap around
        diff = new_self_trigger_count - self_trigger_count - 1;
      }
      pflib_log(info) << "single-shot gate ignored "
                      << diff
                      << " self-triggers while acquiring, decoding, and serializing data"
                      << " for event " << i_event;
    }
    self_trigger_count = new_self_trigger_count;
  }

  tgt->fc().fc_enables(l1aen, extl1a);
  trig->enable_single_shot(og_single_shot);
}
