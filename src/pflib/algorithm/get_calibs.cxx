#include "pflib/algorithm/get_calibs.h"

#include <algorithm>
#include <cmath>

#include "pflib/DecodeAndBuffer.h"
#include "pflib/utility/efficiency.h"
#include "pflib/utility/string_format.h"

namespace pflib::algorithm {

std::array<int, 72> get_calibs(Target* tgt, ROC roc, int& target_adc) {
  static auto the_log_{::pflib::logging::get("get_calibs")};
  std::array<int, 72> calibs;
  /// reserve a vector of the appropriate size to avoid repeating allocation
  /// time for all 72 channels
  DecodeAndBuffer buffer{1};
  for (int ch{0}; ch < 72; ch++) {
    // Set up for highrange charge injection on channel
    pflib_log(info) << "Getting calib for channel " << ch;
    int i_link = ch / 36;
    auto refvol_page =
        pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link);
    auto ch_page = pflib::utility::string_format("CH_%d", ch);
    // We don't want TOT to trigger because our ADC will go to -1. We therefore set
    // the TOA_VREF to 0. This is not pretty but I couldn't find any other parameter
    // to set to turn of the TOT overwriting the ADC.
    auto test_param_handle = roc.testParameters()
                                 .add(refvol_page, "INTCTEST", 1)
                                 .add(refvol_page, "CHOICE_CINJ", 1)
                                 .add(ch_page, "HIGHRANGE", 1)
                                 .add(refvol_page, "TOA_VREF", 0)
                                 .apply();
    tgt->setup_run(1, Target::DaqFormat::SIMPLEROC, 1);
    // Here we're starting at a value well above the saturation of the adc
    // Would prefer to set this in reference to the pedestal for extra safety.
    int calib = 1500;
    int nr_bx = 3;
    while (true) {
      pflib_log(info) << "Testing calib = " << calib;
      auto calib_handle =
          roc.testParameters().add(refvol_page, "CALIB", calib).apply();
      usleep(10);
      std::vector<int> adcs;
      auto central_charge_to_l1a = tgt->fc().fc_get_setup_calib();
      // We need to scan different BXs because the max adc is not neccessarilly in
      // the first one. Need to add phase scan here as well.
      // Currently the bx scan is not working for some reason. Needs a fix.
      //for (int bx = 0; bx < nr_bx; bx++) {
      //  tgt->fc().fc_setup_calib(central_charge_to_l1a + bx);
      //  usleep(10);
      //  tgt->daq_run("CHARGE", buffer, 1, 100);
      //  auto data = buffer.get_buffer();
      //  for (std::size_t i{0}; i < data.size(); i++) {
      //    adcs.push_back(data[i].channel(ch).adc());
      //  }
      //}
      tgt->daq_run("CHARGE", buffer, 1, 100);
      auto data = buffer.get_buffer();
      for (std::size_t i{0}; i < data.size(); i++) {
        adcs.push_back(data[i].channel(ch).adc());
      }
      int max_adc = *std::max_element(adcs.begin(), adcs.end());
      if (std::abs(max_adc - target_adc) <= 1) {
        calibs[ch] = calib;
        pflib_log(info) << "Final calib = " << calib;
        break;
      } else if (max_adc > target_adc) {
        calib -= 50;
        continue;
      } else if (max_adc < target_adc) {
        calib += 1;
      }
    }
  }
  pflib_log(info) << "Calib retrieved for all channels";
  return calibs;
}

}  // namespace pflib::algorithm
