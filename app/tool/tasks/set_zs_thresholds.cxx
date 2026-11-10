#include "set_zs_thresholds.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cmath>
#include <fstream>

#include "../daq_run.h"
#include "pflib/Compile.h"

ENABLE_LOGGING();

using Settings = std::map<std::string, std::map<std::string, uint64_t>>;

// lpGBT I2C moves at most 16 bytes per transaction, 2 of them the address
static constexpr std::size_t MAX_CHUNK = 14;

// call f(start, n) for runs of contiguous addresses, at most MAX_CHUNK long
template <typename F>
static void for_each_chunk(const std::map<int, uint8_t>& regs, F f) {
  int start{-1}, n{0};
  for (const auto& [addr, val] : regs) {
    if (start >= 0 and addr == start + n and n < MAX_CHUNK) {
      n++;
      continue;
    }
    if (start >= 0) f(start, n);
    start = addr;
    n = 1;
  }
  if (start >= 0) f(start, n);
}

// read-modify-write like ECON::applyParameters, but with the ZS-aware map
// and in transfers the lpGBT I2C master can do
static void apply_with_zs(pflib::ECON& econ, const Settings& settings) {
  auto compiler = pflib::Compiler::get("econd::with_zs");
  auto touched = compiler.compile(settings);
  std::map<int, std::map<int, uint8_t>> chip_reg;
  for_each_chunk(touched[0], [&](int start, int n) {
    auto vals = econ.getValues(start, n);
    for (int i{0}; i < n; i++) chip_reg[0][start + i] = vals.at(i);
  });
  for (const auto& [page, vals] : settings) {
    for (const auto& [name, val] : vals) {
      compiler.compile(pflib::upper_cp(page), pflib::upper_cp(name), val,
                       chip_reg);
    }
  }
  for_each_chunk(chip_reg[0], [&](int start, int n) {
    std::vector<uint8_t> vals;
    for (int i{0}; i < n; i++) vals.push_back(chip_reg[0][start + i]);
    econ.setValues(start, vals);
  });
}

void set_zs_thresholds(Target* tgt) {
  auto& econ = tgt->econ(pftool::state.iecon);
  if (econ.type() != "econd") {
    PFEXCEPTION_RAISE("BadEcon", "ECON " + std::to_string(pftool::state.iecon) +
                                     " is not an ECON-D");
  }
  auto source = pftool::readline("Pedestal source (RUN, CALIB) or YAML: ",
                                 {"RUN", "CALIB", "YAML"}, "RUN");

  Settings settings;
  if (source == "YAML") {
    auto fname = pftool::readline("ZS settings file: ");
    YAML::Node node;
    try {
      node = YAML::LoadFile(fname);
    } catch (const YAML::Exception& e) {
      PFEXCEPTION_RAISE("BadFile", "Unable to load " + fname);
    }
    for (const auto& page : node) {
      for (const auto& param : page.second) {
        settings[page.first.as<std::string>()][param.first.as<std::string>()] =
            param.second.as<uint64_t>();
      }
    }
    apply_with_zs(econ, settings);
    pflib_log(info) << "applied " << fname << " to ECON "
                    << pftool::state.iecon;
    return;
  }

  int nevents{0};
  YAML::Node expected;
  if (source == "RUN") {
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
  if (source == "RUN") {
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
  const auto& roc_half_to_erx = tgt->getHardwareRocErxMappingDAQ();
  auto& zs = settings["ZS"];
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
  settings["ROCDAQCTRL"]["GLOBAL_PASS_THRU_MODE"] = zs_on ? 0 : 1;
  apply_with_zs(econ, settings);

  YAML::Node node;
  for (const auto& [page, vals] : settings) {
    for (const auto& [name, val] : vals) node[page][name] = val;
  }
  std::ofstream f{out};
  f << node << "\n";
  pflib_log(info) << "ZS thresholds written to ECON " << pftool::state.iecon
                  << " and " << out << ", pass-through " << (zs_on ? 0 : 1);
}
