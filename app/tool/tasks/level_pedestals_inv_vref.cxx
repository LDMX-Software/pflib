#include "level_pedestals_inv_vref.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

#include "../algorithm/level_pedestals.h"
#include "../algorithm/inv_vref_align.h"

static YAML::Emitter settings_to_yaml(
    const std::map<std::string, std::map<std::string, uint64_t>>& settings) {
  YAML::Emitter out;
  out << YAML::BeginMap;
  for (const auto& page : settings) {
    out << YAML::Key << page.first;
    out << YAML::Value << YAML::BeginMap;
    for (const auto& param : page.second) {
      out << YAML::Key << param.first << YAML::Value << param.second;
    }
    out << YAML::EndMap;
  }
  out << YAML::EndMap;
  return out;
}

void level_pedestals_inv_vref(Target* tgt) {
  auto roc = tgt->roc(pftool::state.iroc);

  // 1) Level pedestals and APPLY automatically
  auto ped_settings = pflib::algorithm::level_pedestals(tgt, roc);
  roc.applyParameters(ped_settings);

  // 2) Run inv_vref scan in-memory (defaults requested)
  auto scan = pflib::algorithm::inv_vref_scan_data(tgt, roc,
                                                   /*nevents=*/100,
                                                   /*ch0=*/17,
                                                   /*ch1=*/51,
                                                   /*step=*/20,
                                                   /*max_vref=*/600);

  // 3) Compute inv_vref settings and APPLY automatically (target=100)
  auto vref_settings = pflib::algorithm::inv_vref_align(scan, /*target_adc=*/100);
  roc.applyParameters(vref_settings);

  // 4) Merge for YAML output
  // Start with pedestal settings, then overlay inv_vref pages
  auto combined = ped_settings;
  for (const auto& page : vref_settings) {
    combined[page.first] = page.second;
  }

  // If you truly want only INV_VREF/TRIM_INV/DACB, you can filter here.
  // BUT: DACB sometimes requires SIGN_DAC=1 to be meaningful when reapplying YAML.
  // I recommend keeping SIGN_DAC when present.
  //
  // If you want strict filtering, uncomment below:
  /*
  std::map<std::string, std::map<std::string, uint64_t>> filtered;
  for (const auto& page : combined) {
    for (const auto& kv : page.second) {
      if (kv.first == "INV_VREF" || kv.first == "TRIM_INV" || kv.first == "DACB" || kv.first == "SIGN_DAC") {
        filtered[page.first][kv.first] = kv.second;
      }
    }
  }
  combined = std::move(filtered);
  */

  auto out = settings_to_yaml(combined);

  // 5) Save YAML (no prompts; deterministic default name)
  std::string fname = pftool::readline_path(
      "level-pedestals-inv-vref-roc-" + std::to_string(pftool::state.iroc) + "-settings",
      ".yaml");

  std::ofstream f{fname};
  if (!f.is_open()) {
    PFEXCEPTION_RAISE("File", "Unable to open file " + fname + ".");
  }
  f << out.c_str() << std::endl;

  // Optional: also print summary
  std::cout << "Applied pedestal leveling and INV_VREF alignment.\n"
            << "Wrote combined settings to " << fname << "\n";
}
