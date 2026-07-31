#include "algo.h"
#include "pflib/TRIG.h"

ENABLE_LOGGING();

void algo(const std::string& cmd, Target* target) {
  /**
   * TRIG.ALGO commands
   *
   * - SPY: view last captured trig algo output
   * - CONFIG: change settings of trigger algorithm
   * - STATUS: print settings of trigger algo and if output is available
   */
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
    params[0] =
        pftool::readline_int("veto mask for recent history: ", params[0], true);
    for (int i{0}; i < 8; i++) {
      std::stringstream prompt;
      prompt << "threshold for channel " << i;
      params[i + 1] = pftool::readline_int(prompt.str(), params[i + 1], true);
    }
    trig->setup_algo(params);
  }
  if (cmd == "STATUS") {
    auto params = trig->get_algo_setup();
    printf("veto mask for recent history: 0x%02x\n", params[0]);
    printf("thresholds (0 -> disable channel)\n");
    for (int i{0}; i < 8; i++) {
      printf("  %d : 0x%02x\n", i, params[i + 1]);
    }
    printf("algo output available? %s\n",
           trig->is_algo_output_available() ? "yes" : "no");
  }
}
