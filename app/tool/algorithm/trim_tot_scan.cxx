#include "trim_tot_scan.h"

#include <cmath>

#include "../daq_run.h"
#include "pflib/utility/efficiency.h"
#include "pflib/utility/string_format.h"

namespace pflib::algorithm {

template <class EventPacket>
std::array<int, 72> trim_tot_scan(Target* tgt, ROC& roc, size_t& n_events,
                                  std::array<int, 72>& calibs,
                                  std::array<int, 2>& tot_vrefs,
                                  std::array<int, 72>& tot_trims) {
  static auto the_log_{::pflib::logging::get("trim_tot_scan")};

  // Iterate through each channel. For each channel, trim down the tot threshold
  // and set the parameter to the point which minimizes abs(tot_efficiency-0.5).

  auto tot_vref_handle = roc.testParameters();
  for (int i{0}; i < 2; i++) {
    auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", i);
    tot_vref_handle.add(refvol_page, "TOT_VREF", tot_vrefs[i])
        .add(refvol_page, "INTCTEST", 1)
        .add(refvol_page, "CHOICE_CINJ", 1);
  }
  auto tot_vref_params = tot_vref_handle.apply();

  DecodeAndBuffer<EventPacket> buffer{n_events,
                                      2};  // working in buffer, not in writer

  for (int ch{0}; ch < 72; ch++) {
    pflib_log(info) << "scanning channel " << ch;
    int target{0};
    // Set the calib value
    int i_link = ch / 36;
    auto refvol_page =
        pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link);
    auto ch_page = pflib::utility::string_format("CH_%d", ch);
    auto calib_handle = roc.testParameters()
                            .add(refvol_page, "CALIB", calibs[ch])
                            .add(ch_page, "HIGHRANGE", 1)
                            .apply();
    // Run the threshold scan TP50 that gives the tot_vref value as output
    for (int trim_tot{0}; trim_tot < 64; trim_tot++) {
      pflib_log(info) << "testing trim_tot = " << trim_tot;
      auto test_handle =
          roc.testParameters().add(ch_page, "TRIM_TOT", trim_tot).apply();
      usleep(10);
      daq_run(tgt, "CHARGE", buffer, n_events, 100);
      auto data = buffer.get_buffer();
      std::vector<int> tots(data.size());
      for (std::size_t i{0}; i < tots.size(); i++) {
        if constexpr (std::is_same_v<
                          EventPacket,
                          pflib::packing::MultiSampleECONDEventPacket>) {
          tots[i] = data[i].samples[data[i].i_soi].channel(i_link, ch).tot();
        } else if constexpr (std::is_same_v<
                                 EventPacket,
                                 pflib::packing::SingleROCEventPacket>) {
          tots[i] = data[i].channel(ch).tot();
        } else {
          PFEXCEPTION_RAISE("BadConf",
                            "Unable to get tot for the configured format");
        }
      }
      auto efficiency = pflib::utility::efficiency(tots);
      pflib_log(info) << "tot efficiency is " << efficiency;
      if (efficiency < 0.5) {
        target = trim_tot;
        continue;
      } else {
        break;  // We want the threshold slightly below 0.5, so use last val.
      }
    }
    pflib_log(info) << "Final trim_tot = " << target;
    tot_trims[ch] = target;
  }
  pflib_log(info) << "Trim_tot retrieved for all channels";
  return tot_trims;
}

template std::array<int, 72>
trim_tot_scan<pflib::packing::SingleROCEventPacket>(
    Target* tgt, ROC& roc, size_t& n_events, std::array<int, 72>& calibs,
    std::array<int, 2>& tot_vrefs, std::array<int, 72>& tot_trims);

template std::array<int, 72>
trim_tot_scan<pflib::packing::MultiSampleECONDEventPacket>(
    Target* tgt, ROC& roc, size_t& n_events, std::array<int, 72>& calibs,
    std::array<int, 2>& tot_vrefs, std::array<int, 72>& tot_trims);

}  // namespace pflib::algorithm
