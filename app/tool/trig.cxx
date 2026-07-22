/**
 * @file trig.cxx
 * TRIG menu commands
 */
#include "pflib/TRIG.h"

#include "pftool.h"

#include <optional>

#include "pflib/packing/Hex.h"
#include "pflib/packing/MultiSampleECONDEventPacket.h"
#include "pflib/utility/string_format.h"

ENABLE_LOGGING();

void trig_render(Target* tgt) {}

/**
 * The econt_buffer_manager firmware block inserts a pair of headers
 * with the same format for the data capture along the trigger path
 * and the capture of the output of the trigger algorithm.
 */
class ECONTCaptureHeader {
  int version_;
  int econ_id_;
  int pre_samples_;
  int n_samples_;
  int length_;
  int n_links_;
 public:
  void from(std::span<uint32_t> data) {
    if (data.size() < 2) {
      PFEXCEPTION_RAISE("MalForm",
        "ECON-T capture packet header requires 2 words");
    }
    /**
     * two 32b headers are added by the firmware
     * 
     * first word
     * - [32:28] = version
     * - [27:18] = econ_id
     * - [7:0] = size
     *
     * second word
     * - [27:24] = number of links
     * - [22:18] = number of presamples
     * - [17:12] = number of samples
     */
    version_ = ((data[0] >> 28) & 0xf);
    econ_id_ = ((data[0] >> 18) & 0x3ff);
    length_ = (data[0] & 0xff);
    n_links_ = ((data[1] >> 24) & 0xf);
    pre_samples_ = ((data[1] >> 18) & 0x1f);
    n_samples_ = ((data[1] >> 12) & 0x3f);
    if (length_ != 2 + n_links_*n_samples_) {
      PFEXCEPTION_RAISE("BadForm",
        "First header reports a length of '"+std::to_string(length_)
        +"' which does not equal the expected value for STC4");
    }
  }

  int version() const {
    return version_;
  }

  int econ_id() const {
    return econ_id_;
  }

  int length() const {
    return length_;
  }

  int n_links() const {
    return n_links_;
  }

  int pre_samples() const {
    return pre_samples_;
  }
  
  int n_samples() const {
    return n_samples_;
  }
};

/**
 * A capture frame has one or more SingleECONTSamples
 * along with two headers written by the firmware that
 * encode how many samples there are and which are important
 */
class SingleECONTCaptureFrame {
 public:
  SingleECONTCaptureFrame() = default;
  SingleECONTCaptureFrame(std::span<uint32_t> data) {
    from(data);
  }
  /**
   * Each ECON-T sample has the following struture,
   * assuming
   * - using STC4 algorithm
   * - with the 5M+4E encoding
   * - and three eTx enabled
   *
   * This means there are up to 8 STCs.
   * Each sample is spread across the three eTx.
   *
   * eTx 0
   * - [31:28] = header
   * - [27:26] = Max1 (index of highest TC in STC1)
   * - [25:24] = Max2 (index of highest TC in STC2)
   * - ... continue down
   * - [12:11] = Max8 (index of highest TC in STC8)
   * - [10:2] = STC1
   * - [1:0] = 2 MSBs of STC2
   * eTx 1
   * - [31:25] = 7 LSBs of STC2
   * - [25:16] = STC3
   * - [15:7] = STC4
   * - [6:0] = 7 MSB of STC5
   * eTx 2
   * - [31:30] = 2 LSB of STC5
   * - [29:21] = STC6
   * - [20:12] = STC7
   * - [11:3] = STC8
   * - [2:0] = zero padding
   */
  class SingleECONTSample {
    static constexpr std::size_t N_STC = 8;
    std::array<int, N_STC> max_tc_;
    std::array<int, N_STC> stc_sums_;
    int bx_;
   public:
    void from(std::span<uint32_t> data) {
      if (data.size() != 3) {
        PFEXCEPTION_RAISE("BadForm",
          "Data received by SingleECONTSample is not length 3.");
      }
      bx_ = ((data[0] >> 28) & 0xf);
      for (int i{0}; i < N_STC; i++) {
        // max is all within the first 32b word even if there
        // are more eTx
        max_tc_[i] = ((data[0] >> (26 - 2*i)) & 0x3);
      }

      // just hardcoding 3 eTx for now
      stc_sums_[0] = ((data[0] >> 2) & 0x1ff);
      stc_sums_[1] = (((data[0] & 0x11) << 7) | ((data[1] >> 25) & 0x7f));
      stc_sums_[2] = ((data[1] >> 16) & 0x1ff);
      stc_sums_[3] = ((data[1] >> 7 ) & 0x1ff);
      stc_sums_[4] = (((data[1] & 0x7f) << 2) | ((data[2] >> 30) & 0x3));
      stc_sums_[5] = ((data[2] >> 21) & 0x1ff);
      stc_sums_[6] = ((data[2] >> 12) & 0x1ff);
      stc_sums_[7] = ((data[2] >>  3) & 0x1ff);
    }

    int bx() const {
      return bx_;
    }

    int stc_sum(int i_stc) const {
      return stc_sums_.at(i_stc);
    }
  };

  void from(std::span<uint32_t> data) {
    header_.from(data);

    if (header_.length() != data.size()) {
      PFEXCEPTION_RAISE("BadForm",
        "First header reports a length of '"+std::to_string(header().length())
        +"' but data.size() = "+std::to_string(data.size()));
    }

    /**
     * and then, with ECON-T configured to be STC4 with
     * 5M+4E encoding, we then see three 32b words for each
     * sample (i.e. size should equal 2 + 3 * samples).
     */
    samples_.resize(header_.n_samples());
    for (int i_sample{0}; i_sample < samples_.size(); i_sample++) {
      samples_[i_sample].from(data.subspan(2 + 3*i_sample, 3));
    }
  }

  const SingleECONTSample& sample(std::optional<int> i_sample = {}) const {
    return samples_.at(i_sample.value_or(header().pre_samples()));
  }

  int bx(std::optional<int> i_sample = {}) const {
    return sample(i_sample).bx();
  }

  int stc_sum(int i_stc, std::optional<int> i_sample = {}) const {
    return sample(i_sample).stc_sum(i_stc);
  }

  const ECONTCaptureHeader& header() const {
    return header_;
  }

  int version() const {
    return header().version();
  }

  int econ_id() const {
    return header().econ_id();
  }
  
  int pre_samples() const {
    return header().pre_samples();
  }

  std::size_t n_samples() const {
    return samples_.size();
  }

 private:
  ECONTCaptureHeader header_;
  std::vector<SingleECONTSample> samples_;
};

class TrigAlgoOutput {
  struct SingleBXOutput {
    std::bitset<8> is_high_peak_;
    bool trigger_;
  };

  const SingleBXOutput& sample(std::optional<int> i_sample = {}) const {
    return samples_.at(i_sample.value_or(header_.pre_samples()));
  }

  std::vector<SingleBXOutput> samples_;
  ECONTCaptureHeader header_;
 public:
  TrigAlgoOutput() = default;
  TrigAlgoOutput(std::span<uint32_t> data) {
    from(data);
  }
  void from(std::span<uint32_t> data) {
    header_.from(data);
    if (header_.length() != data.size()) {
      PFEXCEPTION_RAISE("BadForm",
        "Header reports a length of '"+std::to_string(header_.length())
        +"' but data.size() = "+std::to_string(data.size()));
    }
    samples_.resize(header_.n_samples());
    for (int i_sample{0}; i_sample < samples_.size(); i_sample++) {
      samples_[i_sample].is_high_peak_ = ((data[i_sample + 2] >> 8) & 0xff);
      samples_[i_sample].trigger_ = ((data[i_sample + 2] & 0x1) == 1);
    }
  }

  std::size_t n_samples() const {
    return samples_.size();
  }
  
  bool is_high_peak(int i_stc, std::optional<int> i_sample = {}) const {
    return sample(i_sample).is_high_peak_.test(i_stc);
  }

  bool trigger(std::optional<int> i_sample = {}) const {
    return sample(i_sample).trigger_;
  }
};

/**
 * Interaction with Optical links
 *
 * ## Commands
 * - RESET : call reset on the TRIG block
 * - ALIGN_SETUP : setup the alignment capture time
 * - ALIGN_READ : read the alignment capture buffer
 * - ALIGN_DELAY : set the delay for one elink
 * - ELINK_SPY : spy on six TRIG elinks (Elinks::spy)
 * - EVENT_SPY : readout the last captured event (TRIG::read_event)
 * - PIPELINE : change depth of capture buffer for readout
 * - SAMPLES_PER_L1A : number of samples to readout per l1a
 * - PRESAMPLES : number of "pre-samples" to include in readout
 */
void trig(const std::string& cmd, Target* target) {
  static std::string olink_name;
  pflib::TRIG* trig = target->trig();
  if (trig == 0) return;

  if (cmd == "RESET") {
    trig->reset();
  }
  if (cmd == "STATUS") {
    int pipeline{-1}, econ_id{-1}, samples_per_l1a{-1}, presamples{-1};
    trig->get_daq_setup(pipeline, econ_id, samples_per_l1a, presamples);
    std::cout << "pipeline: " << pipeline << "\n"
              << "econ_id : " << econ_id << "\n"
              << "samples_per_l1a: " << samples_per_l1a << "\n"
              << "presamples: " << presamples << "\n"
              << "capture delay: " << trig->get_alignment_capture() << "\n"
              << std::flush;
    for (int ilink{0}; ilink < trig->n_elinks(); ilink++) {
      std::cout << "link " << ilink
                << " bx delay: " << trig->get_bx_delay(ilink) << "\n";
    }
    std::cout << std::flush;
  }
  if (cmd == "PIPELINE" or cmd == "SAMPLES_PER_L1A" or cmd == "PRESAMPLES" or cmd == "ECONID") {
    int pipeline{-1}, econ_id{-1}, samples_per_l1a{-1}, presamples{-1};
    trig->get_daq_setup(pipeline, econ_id, samples_per_l1a, presamples);
    if (cmd == "PIPELINE") {
      pipeline = pftool::readline_int("pipeline depth: ", pipeline);
    } else if (cmd == "SAMPLES_PER_L1A") {
      samples_per_l1a = pftool::readline_int("number of samples per L1A: ", samples_per_l1a);
    } else if (cmd == "PRESAMPLES") {
      presamples = pftool::readline_int("number of pre-samples: ", presamples);
    } else if (cmd == "ECONID") {
      econ_id = pftool::readline_int("set econ id: ", econ_id);
    }
    trig->setup_daq(pipeline, econ_id, samples_per_l1a, presamples);
  }
  if (cmd == "ELINK_SPY") {
    pflib::Elinks& elinks = target->elinks();
    std::vector<std::vector<uint32_t>> spy(6);
    printf("word :");
    for (int ilink{0}; ilink < spy.size(); ilink++) {
      spy[ilink] = elinks.spy(6 + ilink, ilink == 0);
      printf("  Link %2d", ilink);
    }
    printf("\n");
    for (int iword{0}; iword < spy[0].size(); iword++) {
      printf("%4d :", iword);
      for (int ilink{0}; ilink < spy.size(); ilink++) {
        printf(" %08x", spy[ilink][iword]);
      }
      printf("\n");
    }
  }
  if (cmd == "EVENT_SPY") {
    std::vector<uint32_t> event = trig->read_event();
    if (event.empty()) {
      pflib_log(info) << "no event available";
      return;
    }
    for (int iword{0}; iword < event.size(); iword++) {
      uint32_t word{event[iword]};
      printf("%08x\n", word);
    }

    SingleECONTCaptureFrame frame;
    frame.from(event);
    printf("econ_id = %d n_samples = %d\n", frame.econ_id(), frame.n_samples());
    printf("BX STC1 STC2 STC3 STC4 STC5 STC6 STC7 STC8\n");
    for (int i{0}; i < frame.n_samples(); i++) {
      printf("%2d", frame.bx(i));
      for (int j{0}; j < 8; j++) {
        printf(" %4d", frame.stc_sum(j, i));
      }
      printf("\n");
    }
  }
  if (cmd == "ALIGN_SETUP") {
    int value = pftool::readline_int("Alignment capture delay: ",
                                     trig->get_alignment_capture());
    trig->setup_alignment_capture(value);
  }
  if (cmd == "ALIGN_READ") {
    bool do_fc = pftool::readline_bool("Generate LINKRESET_ECONT?", true);
    bool show_raw = pftool::readline_bool(
        "Show raw data [Y] or idle word interpretation [N]? ", false);
    if (do_fc) target->fc().linkreset_econs();
    usleep(2000);
    for (int ilink = 0; ilink < trig->n_elinks(); ilink++) {
      std::vector<uint32_t> val = trig->read_capture_buffer(ilink);
      // see Section 20 of the ECON-T manual,
      // at UMN we have ECON-T-P1 on the ECON Mezzanine
      // each eTx produces two 16-bit IDLE words per BX
      // and an IDLE word looks like
      //  5b BX | 11b Pattern
      // where BX is a BX counter and the 11b Pattern is set by
      // formatterbuffer.global.link_reset_pattern ? or idle_pattern?
      printf("%02d :", ilink);
      for (auto x : val) {
        if (show_raw) {
          printf(" %08x", x);
        } else {
          printf(" | %02d %03x %02d %03x", (x >> (16 + 11)) & 0x1f,
                 (x >> 16) & 0x7ff, (x >> 11) & 0x1f, (x & 0x7ff));
        }
      }
      printf("\n");
    }
  }
  if (cmd == "ALIGN_DELAY") {
    int ilink = pftool::readline_int("Which elink?", 0);
    trig->set_bx_delay(
        ilink, pftool::readline_int("New delay: ", trig->get_bx_delay(ilink)));
  }
  if (cmd == "BUFFER_CLEAR") {
    while (trig->is_event_available()) {
      trig->read_event();
      usleep(100000);
    }
  }
}

void algo(const std::string& cmd, Target* target) {
  pflib::TRIG* trig = target->trig();
  if (trig == 0) return;
  if (cmd == "SPY") {
    std::vector<uint32_t> algo = trig->read_algo_output();
    if (algo.empty()) {
      pflib_log(info) << "no algo output available";
    }
    for (int iword{0}; iword < algo.size(); iword++) {
      uint32_t word{algo[iword]};
      printf("%08x", word);
      // decoding
      printf("\n");
    }
  }
  if (cmd == "BUFFER_CLEAR") {
    while (trig->is_algo_output_available()) {
      trig->read_algo_output();
      usleep(100000);
    }
  }
  if (cmd == "CONFIG") {
    auto params = trig->get_algo_setup();
    params[0] = pftool::readline_int("veto mask for recent history: ", params[0], true);
    for (int i{0}; i < 8; i++) {
      std::stringstream prompt;
      prompt << "threshold for channel " << i;
      params[i+1] = pftool::readline_int(prompt.str(), params[i+1], true);
    }
    trig->setup_algo(params);
  }
  if (cmd == "STATUS") {
    auto params = trig->get_algo_setup();
    printf("veto mask for recent history: 0x%02x\n", params[0]);
    printf("thresholds (0 -> disable channel)\n");
    for (int i{0}; i < 8; i++) {
      printf("  %d : 0x%02x\n", i, params[i+1]);
    }
  }
}

/**
 * TRIG.SETUP.TIMEIN
 */
static void trigger_timein(Target* tgt) {
  pflib::TRIG* trig = tgt->trig();
  if (trig == 0) return;

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
    roc_setup[pflib::utility::string_format("HALFWISE_%d", half)]["ADC_PEDESTAL"] = 255;
    roc_setup[
        pflib::utility::string_format("DIGITALHALF_%d", half)]["ADC_TH"] = 31;
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
  static const int iroc_oi = 0,
                   ch_oi = 0,
                   stc_oi = 6;
  auto roc_inject = tgt->roc(iroc_oi).testParameters()
        .add("CH_0", "LOWRANGE", 1)
        .apply();

  do {
    bool enable_l1a_follow;
    int og_charge_to_l1a;
    tgt->fc().fc_get_setup_calib(og_charge_to_l1a, enable_l1a_follow);

    int charge_to_l1a =
        pftool::readline_int("Calibration to L1A offset?", og_charge_to_l1a);
    tgt->fc().fc_setup_calib(charge_to_l1a, enable_l1a_follow);

    auto dh_page = tgt->roc(iroc_oi).getParameters("DIGITALHALF_0");
    int og_l1offset = dh_page.at("L1OFFSET");
    int l1offset =
        pftool::readline_int("L1Offset on HGCROC?", og_l1offset);
    auto test_l1offset_handle = tgt->roc(iroc_oi).testParameters()
                                    .add("DIGITALHALF_0", "L1OFFSET", l1offset)
                                    .add("DIGITALHALF_1", "L1OFFSET", l1offset)
                                    .apply();

    int og_pipeline, og_samples_per_l1a, og_presamples, econid;
    trig->get_daq_setup(og_pipeline, econid, og_samples_per_l1a, og_presamples);
    int pipeline{og_pipeline}, samples_per_l1a{og_samples_per_l1a}, presamples{og_presamples};
    pipeline = pftool::readline_int("pipeline: ", pipeline);
    samples_per_l1a = pftool::readline_int("samples_per_l1a: ", samples_per_l1a);
    presamples = pftool::readline_int("presamples: ", presamples);
    trig->setup_daq(pipeline, econid, samples_per_l1a, presamples);

    pflib_log(info)
        << "pedestal runs to confirm alignment and trigger-sum suppression";
    tgt->fc().sendL1A();
    usleep(10000);  // one 100Hz cycle later

    // capture data from this event
    std::vector<uint32_t> trg_pedestal_event = trig->read_event();
    std::vector<uint32_t> pedestal_algo_output_raw = trig->read_algo_output();
    // read_event_sw_headers advances link readout pointer
    std::vector<uint32_t> daq_pedestal_event = tgt->daq().read_event_sw_headers();

    // decode captured data
    SingleECONTCaptureFrame trg_pedestals;
    trg_pedestals.from(trg_pedestal_event);

    TrigAlgoOutput pedestal_algo_output;
    pedestal_algo_output.from(pedestal_algo_output_raw);
    
    pflib::packing::MultiSampleECONDEventPacket daq_pedestals(2);
    daq_pedestals.from(daq_pedestal_event);

    pflib_log(info) << "charge injection run to see non-zero trigger sums in "
                       "specific places";
    tgt->fc().chargepulse();
    usleep(10000);  // one 100Hz cycle later

    // capture data output, using daq last to advance readout pointer
    std::vector<uint32_t> trg_charge_event = trig->read_event();
    std::vector<uint32_t> charge_algo_output_raw = trig->read_algo_output();
    std::vector<uint32_t> daq_charge_event = tgt->daq().read_event_sw_headers();

    // decode after capturing all data so decoding errors don't cause
    // readout pointer misalignment
    SingleECONTCaptureFrame trg_charge;
    trg_charge.from(trg_charge_event);

    TrigAlgoOutput charge_algo_output;
    charge_algo_output.from(charge_algo_output_raw);

    pflib::packing::MultiSampleECONDEventPacket daq_charge(2);
    daq_charge.from(daq_charge_event);

    pflib_log(debug) << "reset charge_to_l1a back to " << og_charge_to_l1a;
    tgt->fc().fc_setup_calib(og_charge_to_l1a, enable_l1a_follow);

    pflib_log(debug) << "reset capture pipeline and n_samples back to original settings";
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
      printf("%2d: %4d %4d -> %4d %4d\n", i_sample,
            daq_pedestals.samples.at(i_sample).channel(i_erx, i_ch).adc_tm1(),
            daq_pedestals.samples.at(i_sample).channel(i_erx, i_ch).adc(),
            daq_charge.samples.at(i_sample).channel(i_erx, i_ch).adc_tm1(),
            daq_charge.samples.at(i_sample).channel(i_erx, i_ch).adc());
    }

    printf("TRG Data\n");
    printf(" i:  ped -> chrg\n");
    for (int i_sample{0}; i_sample < trg_pedestals.n_samples(); i_sample++) {
      int pedestal{trg_pedestals.stc_sum(stc_oi, i_sample)},
          charge{trg_charge.stc_sum(stc_oi, i_sample)};
      printf("%2d: %4d -> %4d%s\n", i_sample, pedestal, charge,
              (i_sample == presamples) ? " <- sample of interest" : "");
    }

    printf("ALGO Output\n");
    printf("       pedestal  ->    charge   \n");
    printf(" i: highpeak trg -> highpeak trg\n");
    for (int i_sample{0}; i_sample < pedestal_algo_output.n_samples(); i_sample++) {
      printf("%2d: ", i_sample);
      for (int i_stc{0}; i_stc < 8; i_stc++) {
        printf("%d", pedestal_algo_output.is_high_peak(i_stc, i_sample));
      }
      printf(" %3d", pedestal_algo_output.trigger(i_sample));
      printf(" -> ");
      for (int i_stc{0}; i_stc < 8; i_stc++) {
        printf("%d", charge_algo_output.is_high_peak(i_stc, i_sample));
      }
      printf(" %3d", charge_algo_output.trigger(i_sample));
      if (i_sample == presamples) printf(" <- sample of interest");
      printf("\n");
    }
  } while (pftool::readline_bool(
      "Want to try another set of timing parameters?", false));
}

void self_trigger(Target* tgt) {
  pflib::TRIG* trig = tgt->trig();
  if (trig == 0) return;

  pflib_log(info) << "setting up parameters for self-trigger test";

  std::map<std::string, std::map<std::string, uint64_t>> roc_setup;
  for (int half{0}; half < 2; half++) {
    roc_setup[pflib::utility::string_format("HALFWISE_%d", half)]["ADC_PEDESTAL"] = 255;
    roc_setup[
        pflib::utility::string_format("DIGITALHALF_%d", half)]["ADC_TH"] = 31;
    auto refvol_page{
        pflib::utility::string_format("REFERENCEVOLTAGE_%d", half)};
    roc_setup[refvol_page]["CALIB"] = 3000;
    roc_setup[refvol_page]["INTCTEST"] = 1;
  }
  auto roc_test_lock = tgt->tempApplyAllROCs(roc_setup);

  static const int iroc_oi = 0,
                   ch_oi = 0,
                   stc_oi = 6;
  auto roc_inject = tgt->roc(iroc_oi).testParameters()
        .add("CH_0", "LOWRANGE", 1)
        .apply();

  bool enable_l1a_follow;
  int charge_to_l1a;
  tgt->fc().fc_get_setup_calib(charge_to_l1a, enable_l1a_follow);
  pflib_log(info) << "charge_to_l1a = " << charge_to_l1a
                  << " enable_l1a_follow = " << enable_l1a_follow;
  bool l1aen, extl1a;
  tgt->fc().fc_enables_read(l1aen, extl1a);
  pflib_log(info) << "l1a_enabled = " << l1aen
                  << " external_l1a = " << extl1a;
  pflib_log(info) << "event occupancy: " << tgt->daq().getEventOccupancy();

  pflib_log(info) << "disabling the L1A following the charge command";
  tgt->fc().fc_setup_calib(charge_to_l1a, false);

  pflib_log(info) << "event occupancy: " << tgt->daq().getEventOccupancy();

  pflib_log(info) << "enabling external L1A";
  tgt->fc().fc_enables(true, true);

  pflib_log(info) << "event occupancy: " << tgt->daq().getEventOccupancy();

  do {
    auto dh_page = tgt->roc(iroc_oi).getParameters("DIGITALHALF_0");
    int og_l1offset = dh_page.at("L1OFFSET");
    int l1offset =
        pftool::readline_int("L1Offset on HGCROC?", og_l1offset);
    auto test_l1offset_handle = tgt->roc(iroc_oi).testParameters()
                                    .add("DIGITALHALF_0", "L1OFFSET", l1offset)
                                    .add("DIGITALHALF_1", "L1OFFSET", l1offset)
                                    .apply();

    int og_pipeline, og_samples_per_l1a, og_presamples, econid;
    trig->get_daq_setup(og_pipeline, econid, og_samples_per_l1a, og_presamples);
    int pipeline{og_pipeline}, samples_per_l1a{og_samples_per_l1a}, presamples{og_presamples};
    pipeline = pftool::readline_int("pipeline: ", pipeline);
    //samples_per_l1a = pftool::readline_int("samples_per_l1a: ", samples_per_l1a);
    //presamples = pftool::readline_int("presamples: ", presamples);
    trig->setup_daq(pipeline, econid, samples_per_l1a, presamples);

    pflib_log(info) << "charge injection";
    tgt->fc().chargepulse();
    usleep(10000);  // one 100Hz cycle later
    pflib_log(info) << "event occupancy: " << tgt->daq().getEventOccupancy();
  
    if (tgt->daq().getEventOccupancy() == 1) {
      // capture data output, using daq last to advance readout pointer
      std::vector<uint32_t> trg_charge_event = trig->read_event();
      std::vector<uint32_t> charge_algo_output_raw = trig->read_algo_output();
      std::vector<uint32_t> daq_charge_event = tgt->daq().read_event_sw_headers();
  
      // decode after capturing all data so decoding errors don't cause
      // readout pointer misalignment
      SingleECONTCaptureFrame trg_charge;
      trg_charge.from(trg_charge_event);
  
      TrigAlgoOutput charge_algo_output;
      charge_algo_output.from(charge_algo_output_raw);
  
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
      for (int i_sample{0}; i_sample < trg_charge.n_samples(); i_sample++) {
        int charge{trg_charge.stc_sum(stc_oi, i_sample)};
        printf("%2d: %4d\n", i_sample, charge);
      }
  
      printf("ALGO Output\n");
      printf(" i: highpeak trg\n");
      for (int i_sample{0}; i_sample < charge_algo_output.n_samples(); i_sample++) {
        printf("%2d: ", i_sample);
        for (int i_stc{0}; i_stc < 8; i_stc++) {
          printf("%d", charge_algo_output.is_high_peak(i_stc, i_sample));
        }
        printf(" %3d", charge_algo_output.trigger(i_sample));
        printf("\n");
      }
    }
    trig->setup_daq(og_pipeline, econid, og_samples_per_l1a, og_presamples);
  } while (pftool::readline_bool(
      "Want to try another set of timing parameters?", false));

  tgt->fc().fc_setup_calib(charge_to_l1a, enable_l1a_follow);
  tgt->fc().fc_enables(l1aen, extl1a);
}

namespace {
// accessing the TRIGGER path only works on the ZCU
// where we have hardware and firmware access to the TRIGGER stream
auto menu_trig =
    pftool::menu("TRIG", "TRIGGER functionalities", trig_render, ONLY_ZCU)
        ->line("STATUS", "printout trigger settings", trig)
        ->line("PIPELINE", "set the pipeline depth for trigger capture", trig)
        ->line("SAMPLES_PER_L1A", "set number of samples to readout in an L1A", trig)
        ->line("PRESAMPLES", "set number of pre-samples in an L1A", trig)
        ->line("ECONID", "set econ id", trig)
        ->line("RESET", "Reset trigger firmware blocks", trig)
        ->line("ALIGN_SETUP", "Setup the alignment delay", trig)
        ->line("ALIGN_READ", "Capture and read the alignment windows", trig)
        ->line("ALIGN_DELAY", "Setup the word delay for an elink", trig)
        ->line("SW_L1A", "send a L1A from software",
               [](Target* tgt) { tgt->fc().sendL1A(); })
        ->line("ADV", "advance the readout pointers",
               [](Target* tgt) { tgt->daq().advanceLinkReadPtr(); })
        ->line("TIMEIN", "scan delay settings to timein trigger capture", trigger_timein)
        ->line("SELF_TRIG", "attempt to trigger on a charge pulse", self_trigger)
        ->line("BUFFER_CLEAR", "clear buffer by reading events until none are left", trig)
        ->line("ELINK_SPY", "spy on the six TRIG elinks", trig)
        ->line("EVENT_SPY", "attempt to read the last captured event", trig);

auto menu_algo =
    menu_trig->submenu("ALGO", "configure and view trigger algorithm")
        ->line("CONFIG", "configure trigger algorithm parameters", algo)
        ->line("SPY", "view output of trigger algorithm", algo)
        ->line("STATUS", "printout algorithm settings and output capture status", algo)
        ->line("BUFFER_CLEAR", "clear out buffer of algo output", algo);
}  // namespace
