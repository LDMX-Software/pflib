#include "set_zs_thresholds.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cmath>
#include <fstream>

#include "../daq_run.h"

ENABLE_LOGGING();

void set_zs_thresholds(Target* tgt) {
  auto& econ = tgt->econ(pftool::state.iecon);
  if (econ.type() != "econd") {
    PFEXCEPTION_RAISE("BadEcon", "ECON " + std::to_string(pftool::state.iecon) +
                                     " is not an ECON-D");
  }
  bool take_run = pftool::readline_bool(
      "Take a pedestal run (otherwise use the calib file)? ", true);
  int nevents{0};
  YAML::Node expected;
  if (take_run) {
    nevents = pftool::readline_int("Number of events: ", 100);
  } else {
    auto fname =
        pftool::readline("Calibration file: ", pftool::state.calib_file);
    try {
      expected = YAML::LoadFile(fname)["expected_pedestal"];
    } catch (const YAML::Exception& e) {
      PFEXCEPTION_RAISE("BadFile", "Unable to load calibration " + fname);
    }
    if (not expected) {
      PFEXCEPTION_RAISE("BadConf", fname + " has no expected_pedestal");
    }
  }
  int margin = pftool::readline_int("Threshold above pedestal T (ADC): ", 10);
  int lambda = pftool::readline_int("ZS lambda (CM weight / 32): ", 0);
  int kappa = pftool::readline_int("ZS kappa (BX-1 weight / 32): ", 0);
  bool zs_on =
      pftool::readline_bool("Turn ZS on (pass-through off) after? ", false);
  auto out = pftool::readline_path("econd-zs", ".yaml");

  // pedestal per (ROC, channel)
  std::map<int, std::vector<double>> ped;
  if (take_run) {
    econ.applyParameter("ROCDAQCTRL", "GLOBAL_PASS_THRU_MODE", 1);
    tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);
    DecodeAndBuffer buffer{static_cast<std::size_t>(nevents), tgt->nrocs() * 2};
    daq_run(tgt, "PEDESTAL", buffer, nevents, pftool::state.daq_rate);
    const auto& data = buffer.get_buffer();
    const auto& mapping = tgt->getRocErxMapping();
    for (int i_roc : tgt->roc_ids()) {
      ped[i_roc].resize(72);
      for (int ch{0}; ch < 72; ch++) {
        auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, ch);
        double sum{0};
        for (const auto& ep : data) sum += ep.soi().channel(i_erx, i_ch).adc();
        ped[i_roc][ch] = data.empty() ? 0 : sum / data.size();
      }
    }
  } else {
    for (int i_roc : tgt->roc_ids()) {
      auto exp = expected["roc" + std::to_string(i_roc)];
      if (not exp or exp.size() != 72) {
        pflib_log(warn) << "no expected pedestal for ROC " << i_roc;
        continue;
      }
      for (int ch{0}; ch < 72; ch++) ped[i_roc].push_back(exp[ch].as<double>());
    }
  }

  // ZS page names use the physical eRx, slot 18 is the calib channel
  const auto roc_half_to_erx = tgt->getHardwareRocErxMappingDAQ();
  std::map<std::string, std::map<std::string, uint64_t>> params;
  auto& zs = params["ZS"];
  for (const auto& [i_roc, peds] : ped) {
    int c_min{255}, c_max{0};
    for (int half{0}; half < 2; half++) {
      int erx = half == 0 ? roc_half_to_erx.at(i_roc).first
                          : roc_half_to_erx.at(i_roc).second;
      std::string pre = std::to_string(erx) + "_ZS_";
      for (int slot{0}; slot < 37; slot++) {
        int c{255};
        if (slot != 18) {
          int ch = 36 * half + (slot < 18 ? slot : slot - 1);
          c = std::clamp<int>(std::lround(peds[ch]) + margin, 0, 255);
          c_min = std::min(c_min, c);
          c_max = std::max(c_max, c);
        }
        std::string s = std::to_string(slot);
        zs[pre + "C_I_" + s] = c;
        zs[pre + "LAMBDA_" + s] = lambda;
        zs[pre + "KAPPA_" + s] = kappa;
      }
    }
    pflib_log(info) << "ROC " << i_roc << ": c from " << c_min << " to "
                    << c_max;
  }
  econ.applyParameters(params);
  params["ROCDAQCTRL"]["GLOBAL_PASS_THRU_MODE"] = zs_on ? 0 : 1;
  econ.applyParameter("ROCDAQCTRL", "GLOBAL_PASS_THRU_MODE", zs_on ? 0 : 1);

  YAML::Node node;
  for (const auto& [page, vals] : params) {
    for (const auto& [name, val] : vals) node[page][name] = val;
  }
  std::ofstream f{out};
  f << node << "\n";
  pflib_log(info) << "ZS thresholds written to ECON " << pftool::state.iecon
                  << " and " << out << ", pass-through " << (zs_on ? 0 : 1);
}
