#include "watch_run.h"

#include "decode_multi_sample.h"
#include "pflib/TRIG.h"
#include "pflib/packing/HcalECONTCaptureFrame.h"
#include "pflib/packing/HcalECONTRocSTCMapping.h"
#include "pflib/packing/Hex.h"
#include "pflib/packing/MultiSampleECONDEventPacket.h"
#include "pflib/packing/TrigAlgoOutput.h"
#include "pflib/utility/string_format.h"
using pflib::packing::HcalECONTCaptureFrame;
using pflib::packing::MultiSampleECONDEventPacket;

ENABLE_LOGGING();

/**
 * configure which ROC channels to readout handle writing the CSV
 */
class WriteROCCHannels {
  std::vector<std::pair<int,int>> channels_to_readout_;
  std::vector<std::pair<int,int>> stcs_to_readout_;
  std::ofstream file_;
 public:
  void reset() {
    channels_to_readout_.clear();
    stcs_to_readout_.clear();
  }
  bool configured() {
    return not channels_to_readout_.empty();
  }
  void add(int i_roc, int i_ch, const pflib::packing::HcalECONTRocSTCMapping& mapping) {
    channels_to_readout_.emplace_back(i_roc, i_ch);
    stcs_to_readout_.emplace_back(mapping.toECONTSTC(i_roc, i_ch));
  }
  bool open(const std::string& filepath) {
    file_.open(filepath);
    if (not file_.is_open()) { return false; }
    // write header line
    file_ << "i_event,i_sample";
    for (const auto& [roc, ch] : channels_to_readout_) {
      file_ << ",roc_" << roc << "_ch_" << ch << "_Tp"
           << ",roc_" << roc << "_ch_" << ch << "_Tc"
           << ",roc_" << roc << "_ch_" << ch << "_adc_tm1"
           << ",roc_" << roc << "_ch_" << ch << "_adc"
           << ",roc_" << roc << "_ch_" << ch << "_toa"
           << ",roc_" << roc << "_ch_" << ch << "_tot";
    }
    for (const auto& [econt, stc] : stcs_to_readout_) {
      file_ << ",econt_" << econt << "_stc_" << stc;
    }
    file_ << '\n';
    return true;
  }
  void serialize(
    const MultiSampleECONDEventPacket& data,
    const std::vector<HcalECONTCaptureFrame>& trig,
    int i_event,
    int n_samples,
    const pflib::packing::SingleECONDRocErxMapping& mapping
  ) {
    for (int i_sample{0}; i_sample < n_samples; i_sample++) {
      file_ << i_event << ',' << i_sample;
      for (const auto& [roc, ch] : channels_to_readout_) {
        auto [i_erx, i_ch] = mapping.toErxChannel(roc, ch);
        auto sample{data.samples.at(i_sample).channel(i_erx, i_ch)};
        file_ << ',' << sample.Tp() << ',' << sample.Tc() << ','
              << sample.adc_tm1() << ',' << sample.adc() << ','
              << sample.toa() << ',' << sample.tot();
      }
      for (const auto& [econt, stc] : stcs_to_readout_) {
        file_ << ',' << trig[i_sample].stc_sum(stc, econt, 0);
      }
      file_ << '\n';
    }
  }
  void close() {
    file_.close();
  }
  friend inline std::ostream& operator<<(std::ostream& o,
                                         const WriteROCCHannels& self) {
    o << "[";
    for (const auto& [i_roc, i_ch] : self.channels_to_readout_) {
      o << " ROC" << i_roc << " CH" << i_ch;
    }
    o << " ]";
    return o;
  }
};

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
  static WriteROCCHannels writer;
  if (writer.configured()) {
    pflib_log(info) << "ROC Channels being readout defined before a prior run: " << writer;
    if (pftool::readline_bool("Change which channels are being read out?", false)) {
      writer.reset();
    }
  }
  if (not writer.configured()) {
    pflib::packing::HcalECONTRocSTCMapping mapping(tgt->getHardwareRocErxMappingTRG());
    for (int i_roc : tgt->roc_ids()) {
      static std::string channel_str = "0,1,2,3,4,5,6,7";
      channel_str = pftool::readline(
        "Comma-separated list of channels in ROC"+std::to_string(i_roc)+"to readout:", channel_str);
      std::stringstream channel_stream{channel_str};
      std::string channel;
      while (getline(channel_stream, channel, ',')) {
        writer.add(i_roc, std::stoi(channel), mapping);
      }
    }
  }
  const auto& mapping{tgt->getRocErxMapping()};

  auto path{pftool::readline_path("watch-run", ".csv")};
  if (not writer.open(path)) {
    pflib_log(fatal) << "unable to open " << path;
    return;
  }

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
    std::vector<HcalECONTCaptureFrame> trg_charge =
        decode_multi_sample<HcalECONTCaptureFrame>(trig->get_l1a_per_ror(),
                                                   trg_charge_event);

    /*
    std::vector<TrigAlgoOutput> charge_algo_output =
        decode_multi_sample<TrigAlgoOutput>(trig->get_l1a_per_ror(),
                                            charge_algo_output_raw);
    */

    pflib::packing::MultiSampleECONDEventPacket daq_charge(2);
    daq_charge.from(daq_charge_event);

    writer.serialize(daq_charge, trg_charge, i_event, trig->get_l1a_per_ror(), mapping);

    int new_self_trigger_count = trig->get_self_trigger_count();
    if (new_self_trigger_count != ((self_trigger_count + 1) % 0xffff)) {
      // self trigger counter is 16bits and so we may have wrapped around
      int diff{0};
      if (new_self_trigger_count < self_trigger_count) {
        // wrap around happend
        diff = (0xffff - self_trigger_count) + new_self_trigger_count;
      } else {
        // no wrap around
        diff = new_self_trigger_count - self_trigger_count - 1;
      }
      pflib_log(info)
          << "single-shot gate ignored " << diff
          << " self-triggers while acquiring, decoding, and serializing data"
          << " for event " << i_event;
    }
    self_trigger_count = new_self_trigger_count;
  }

  tgt->fc().fc_enables(l1aen, extl1a);
  trig->enable_single_shot(og_single_shot);
}
