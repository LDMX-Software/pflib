/**
 * @file trig.cxx
 * TRIG menu commands
 */
#include "pflib/TRIG.h"

#include "pftool.h"

#include <optional>

#include "pflib/packing/Hex.h"
#include "pflib/packing/DAQLinkFrame.h"
#include "pflib/utility/string_format.h"

ENABLE_LOGGING();

void trig_render(Target* tgt) {}

/**
 * A capture frame has one or more SingleECONTSamples
 * along with two headers written by the firmware that
 * encode how many samples there are and which are important
 */
class SingleECONTCaptureFrame {
 public:
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
   * - [27:26] = Max1 (index of highest STC)
   * - [25:24] = Max2 (index of second-highest STC)
   * - ... continue down
   * - [12:11] = Max8 (index of eight-highest STC - aka the lowest)
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
    std::array<int, N_STC> max_;
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
        max_[i] = ((data[0] >> (26 - 2*i)) & 0x3);
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
    int length = (data[0] & 0xff);
    if (length != data.size()) {
      PFEXCEPTION_RAISE("BadForm",
        "First header reports a length of '"+std::to_string(length)
        +"' but data.size() = "+std::to_string(data.size()));
    }
    int n_links = ((data[1] >> 24) & 0xf);
    pre_samples_ = ((data[1] >> 18) & 0x1f);
    int n_samples = ((data[1] >> 12) & 0x3f);
    if (length != 2 + n_links*n_samples) {
      PFEXCEPTION_RAISE("BadForm",
        "First header reports a length of '"+std::to_string(length)
        +"' which does not equal the expected value for STC4");
    }

    /**
     * and then, with ECON-T configured to be STC4 with
     * 5M+4E encoding, we then see three 32b words for each
     * sample (i.e. size should equal 2 + 3 * samples).
     */
    samples_.resize(n_samples);
    for (int i_sample{0}; i_sample < n_samples; i_sample++) {
      samples_[i_sample].from(data.subspan(2 + 3*i_sample, 3));
    }
  }

  const SingleECONTSample& sample(std::optional<int> i_sample = {}) const {
    return samples_.at(i_sample.value_or(pre_samples_));
  }

  int bx(std::optional<int> i_sample = {}) const {
    return sample(i_sample).bx();
  }

  int stc_sum(int i_stc, std::optional<int> i_sample = {}) const {
    return sample(i_sample).stc_sum(i_stc);
  }

  int version() const {
    return version_;
  }

  int econ_id() const {
    return econ_id_;
  }
  
  int pre_samples() const {
    return pre_samples_;
  }

  std::size_t n_samples() const {
    return samples_.size();
  }

 private:
  std::vector<SingleECONTSample> samples_;
  int version_;
  int econ_id_;
  int pre_samples_;
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
      usleep(10000);
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
  static const uint32_t ZERO = 0xa0000000;

  static const uint32_t DAQ_HEADER_PATTERN = 0xf0000005;

  auto& daq{tgt->daq()};

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
   * We then enable charge injection with a specific channel.
   * Just putting it in channel 0 of whichever iroc is selected.
   */
  auto roc_inject = tgt->roc(pftool::state.iroc).testParameters()
        .add("CH_0", "LOWRANGE", 1)
        .apply();

  do {
    int og_charge_to_l1a = tgt->fc().fc_get_setup_calib();
    int charge_to_l1a =
        pftool::readline_int("Calibration to L1A offset?", og_charge_to_l1a);
    tgt->fc().fc_setup_calib(charge_to_l1a);

    /*
    int default_l1offset = 16;
    int l1offset =
        pftool::readline_int("L1Offset on HGCROC?", default_l1offset);
    auto test_l1offset_handle = roc.testParameters()
                                    .add("DIGITALHALF_0", "L1OFFSET", l1offset)
                                    .add("DIGITALHALF_1", "L1OFFSET", l1offset)
                                    .apply();

    int default_global_latency_time = 10;
    int global_latency_time = pftool::readline_int(
        "Global latency time on the HGCROC?", default_global_latency_time);
    auto test_latency_time =
        roc.testParameters()
            .add("MASTERTDC_0", "GLOBAL_LATENCY_TIME", global_latency_time)
            .add("MASTERTDC_1", "GLOBAL_LATENCY_TIME", global_latency_time)
            .apply();
    */

    pflib_log(info) << "storing link settings and expanding capture window";

    /**
     * TODO check this claim
     * The window size in the firmware is stored in 6 bits,
     * so the maximum capture window (and therefore maximum delay)
     * is 63 (2^6 - 1).
     *
     * @note Capture windows larger than 63 seem to be naively trimmed
     * without warning or notice.
     */
    int max_delay = 63;
    int pipeline, samples_per_l1a, presamples, econid;
    trig->get_daq_setup(pipeline, econid, samples_per_l1a, presamples);
    trig->setup_daq(max_delay, econid, max_delay, presamples);

    pflib_log(info)
        << "pedestal runs to confirm alignment and trigger-sum suppression";
    tgt->fc().sendL1A();
    usleep(10000);  // one 100Hz cycle later

    std::vector<uint32_t> pedestal_event = trig->read_event();

    SingleECONTCaptureFrame pedestals;
    pedestals.from(pedestal_event);

    tgt->daq().advanceLinkReadPtr();

    pflib_log(info) << "charge injection run to see non-zero trigger sums in "
                       "specific places";
    tgt->fc().chargepulse();
    usleep(10000);  // one 100Hz cycle later
    std::vector<uint32_t> charge_event = trig->read_event();

    SingleECONTCaptureFrame charge;
    charge.from(charge_event);

    tgt->daq().advanceLinkReadPtr();

    pflib_log(debug) << "reset capture pipeline and n_samples back to original settings";
    trig->setup_daq(pipeline, econid, samples_per_l1a, presamples);

    pflib_log(debug) << "reset charge_to_l1a back to " << og_charge_to_l1a;
    tgt->fc().fc_setup_calib(og_charge_to_l1a);

    pflib_log(info) << "analyze words readout from links";
    pflib_log(debug) << "delay : pedestal -> charge";
    std::array<int, 6> delays{-1, -1, -1, -1, -1, -1};
    std::array<std::pair<int, int>, 4> daq_pedestal_charge_adc;
    std::array<std::pair<int, int>, 2> daq_ev;
    for (int i_sample{0}; i_sample < pedestals.n_samples(); i_sample++) {
      printf("%2d -> ", i_sample);
      for (int i_stc{0}; i_stc < 8; i_stc++) {
        printf(" %4d", charge.stc_sum(i_stc, i_sample) - pedestals.stc_sum(i_stc, i_sample));
      }
      printf("\n");
    }
  } while (pftool::readline_bool(
      "Want to try another set of timing parameters?", false));
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
        ->line("BUFFER_CLEAR", "clear buffer by reading events until none are left", trig)
        ->line("ELINK_SPY", "spy on the six TRIG elinks", trig)
        ->line("EVENT_SPY", "attempt to read the last captured event", trig);

auto menu_algo =
    menu_trig->submenu("ALGO", "configure and view trigger algorithm")
        ->line("CONFIG", "configure trigger algorithm parameters", algo)
        ->line("SPY", "view output of trigger algorithm", algo)
        ->line("STATUS", "printout algorithm settings and output capture status", algo);
}  // namespace
