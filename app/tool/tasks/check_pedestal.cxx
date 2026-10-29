#include "check_pedestal.h"

#include <yaml-cpp/yaml.h>

#include <cmath>

#include "../daq_run.h"
#include "pflib/utility/median.h"

ENABLE_LOGGING();

void check_pedestal(Target* tgt) {
  auto fname =
      pftool::readline("Calibration file: ", pftool::state.calib_file);
  int nevents = pftool::readline_int("Number of events: ", 100);
  int max_off = pftool::readline_int("Flag channels off by more than (ADC): ", 10);

  YAML::Node expected;
  try {
    expected = YAML::LoadFile(fname)["expected_pedestal"];
  } catch (const YAML::Exception& e) {
    PFEXCEPTION_RAISE("BadFile", "Unable to load calibration " + fname);
  }
  if (not expected) {
    PFEXCEPTION_RAISE("BadConf", fname + " has no expected_pedestal");
  }

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);
  DecodeAndBuffer buffer{static_cast<std::size_t>(nevents), tgt->nrocs() * 2};
  daq_run(tgt, "PEDESTAL", buffer, nevents, pftool::state.daq_rate);
  const auto& data = buffer.get_buffer();
  const auto& mapping = tgt->getRocErxMapping();

  int n_off_total{0};
  for (int i_roc : tgt->roc_ids()) {
    auto exp = expected["roc" + std::to_string(i_roc)];
    if (not exp or exp.size() != 72) {
      pflib_log(warn) << "no expected pedestal for ROC " << i_roc;
      continue;
    }
    std::vector<int> diffs;
    std::vector<std::string> off;
    for (int ch{0}; ch < 72; ch++) {
      auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, ch);
      std::vector<int> adcs;
      adcs.reserve(data.size());
      for (const auto& ep : data) adcs.push_back(ep.soi().channel(i_erx, i_ch).adc());
      int diff = pflib::utility::median(adcs) - std::lround(exp[ch].as<double>());
      diffs.push_back(diff);
      if (std::abs(diff) > max_off) {
        off.push_back(std::to_string(ch) + " (" + std::to_string(diff) + ")");
      }
    }
    std::string off_list;
    for (const auto& s : off) off_list += " " + s;
    pflib_log(info) << "ROC " << i_roc << ": median offset "
                    << pflib::utility::median(diffs) << " ADC, " << off.size()
                    << " channels off by more than " << max_off
                    << (off.empty() ? "" : ":" + off_list);
    n_off_total += off.size();
  }
  if (n_off_total == 0) {
    pflib_log(info) << "pedestal matches " << fname;
  } else {
    pflib_log(warn) << n_off_total << " channels differ from " << fname;
  }
}
