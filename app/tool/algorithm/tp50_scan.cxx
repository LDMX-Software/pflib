#include <vector>

#include "../daq_run.h"
#include "pflib/utility/efficiency.h"
#include "pflib/utility/string_format.h"

namespace pflib::algorithm {

double eff_scan(Target* tgt, ROC& roc, int& channel, int& vref_value, int& n_events,
                auto& refvol_page,
                auto& buffer) {  // will the script understand auto refvol_page
                                // and auto buffer?
  static auto the_log_{::pflib::logging::get("eff_scan")};
  auto vref_test_param = roc.testParameters()
                             .add(refvol_page, "TOT_VREF", vref_value)
                             .apply();  // applying relevant parameters
  usleep(10);

  // daq run
  tgt->daq_run("CHARGE", buffer, n_events, 100);

  auto data = buffer.get_buffer();
  std::vector<double> tot_list;
  for (std::size_t i{0}; i < data.size(); i++) {
    auto tot = data[i].channel(channel).tot();
    if (tot >= 0) {  // tot = -1 when it is not triggered
      tot_list.push_back(tot);
    }
  }

  double tot_eff = static_cast<double>(tot_list.size()) /
                   n_events;  // calculating tot efficiency
  return tot_eff;
}

int global_vref_scan(Target* tgt, ROC& roc, int& channel, int& n_events,
                     auto& refvol_page, auto& buffer) {
  static auto the_log_{::pflib::logging::get("tp50_scan")};
  std::vector<double> tot_eff_list;
  std::vector<int> vref_list = {0, 600};
  int vref_value{100000};
  double tol{0.3};
  int max_its = 40;

  while (true) {
    if (std::abs(vref_list.back() - vref_list.front()) <= 1) {
      pflib_log(info) << "Search converged";
      break;
    }
    // Bisectional search
    if (!tot_eff_list.empty()) {
      if (tot_eff_list.back() > 0.5) {
        vref_list.front() = vref_value;
      } else {
        vref_list.back() = vref_value;
      }
      vref_value = (vref_list.back() + vref_list.front()) / 2;
    } else {
      vref_value = (vref_list.back() + vref_list.front()) / 2;
    }
    pflib_log(info) << "the vref value is " << vref_value;
    double efficiency =
        eff_scan(tgt, roc, channel, vref_value, n_events, refvol_page, buffer);
    pflib_log(info) << "tot efficiency is " << efficiency;
    if (std::abs(efficiency - 0.5) < tol) {
      pflib_log(info) << "Efficiency within tolerance!";
      break;
    }

    if (vref_value == 0 || vref_value == 600) {
      pflib_log(info) << "No tp_50 was found! Channel " << channel;
      vref_value = -1;
      break;
    }

    tot_eff_list.push_back(
        efficiency);  // vref with tot_eff was found, but tot_eff - 0.5 !< tol

    // if the tot_eff is consistently above tolerance, we limit the iterations
    if (tot_eff_list.size() > max_its) {
      pflib_log(info) << "Ended after " << max_its << " iterations!" << '\n'
                      << "Final vref is : " << vref_value
                      << " with tot_eff = " << efficiency
                      << " for channel : " << channel;
      break;
    }
  }
  pflib_log(info) << "Final vref value is " << vref_value;
  return vref_value;
}

int local_vref_scan(Target* tgt, ROC& roc, int& channel, int& vref_value,
                      int& n_events, auto& refvol_page, auto& buffer) {
  // Increase vref value until eff < 0.5
  static auto the_log_{::pflib::logging::get("local_vref_scan")};
  for (int vref = vref_value; vref <= 600; vref++) {
    pflib_log(info) << "Testing vref = " << vref;
    double efficiency =
        eff_scan(tgt, roc, channel, vref, n_events, refvol_page, buffer);
    if (efficiency < 0.5) {
      return vref;
    }
  }
  pflib_log(info) << "No vref found for this channel!";
  return vref_value;
}

template <class EventPacket>
std::array<int, 2> tp50_scan(Target* tgt, ROC& roc, int& n_events,
                            std::array<int, 72>& calib, std::array<int, 2>& link_vref_list) 
  {
  static auto the_log_{::pflib::logging::get("tp50_scan")};

  link_vref_list = {-1, -1};  // results array, which will hold the best vref for each link

  for (int channel{0}; channel < 72; channel++) {
    pflib_log(info) << "Scanning channel " << channel << " for tp50, "
                    << "using CALIB " << calib[channel];

    int link = channel / 36;
    auto channel_page = pflib::utility::string_format("CH_%d", channel);
    auto refvol_page =
        pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
    auto test_param_handle = roc.testParameters()
                                 .add(refvol_page, "INTCTEST", 1)
                                 .add(refvol_page, "CHOICE_CINJ", 1)
                                 .add(refvol_page, "CALIB", calib[channel])
                                 .add(channel_page, "HIGHRANGE", 1)
                                 .apply();

    DecodeAndBuffer<EventPacket> buffer{n_events, 2};

    int vref = link_vref_list[link];

    if (vref == -1) {
      pflib_log(info) << "Doing a global scan";
      vref = global_vref_scan(tgt, roc, channel, n_events, refvol_page, buffer);
      link_vref_list[link] = vref;
      pflib_log(info) << "vref = " << vref;
      continue;
    }

    double channel_eff =
        eff_scan(tgt, roc, channel, vref, n_events, refvol_page, buffer);

    if (channel_eff < 0.5) {
      pflib_log(info) << "Channel accounted for by previous vref!"
                      << " The vref value is " << vref << " and the eff is "
                      << channel_eff;
      continue;
    } else {
      pflib_log(info)
          << "Scanning vref's local neighbourhood for a more suitable vref...";
      int channel_vref = local_vref_scan_2(tgt, roc, channel, vref, n_events,
                                           refvol_page, buffer);
      pflib_log(info) << "New vref is " << channel_vref;
      link_vref_list[link] = channel_vref;
    }
  }
  return link_vref_list;
}
}  // namespace pflib::algorithm
