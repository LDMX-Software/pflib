#include "vt50_scan.h"

#include <nlohmann/json.hpp>

#include "../daq_run.h"
#include "../econ_links.h"
#include "pflib/utility/string_format.h"

ENABLE_LOGGING();

// helper function to facilitate EventPacket dependent behaviour
template <class EventPacket>
static void vt50_scan_writer(Target* tgt, pflib::ROC& roc, size_t nevents,
                             bool& preCC, bool& highrange, bool& search,
                             int& channel, int& toa_threshold, int& vref_lower,
                             int& vref_upper, int& nsteps, std::string& fname,
                             int& link, std::string& vref_page,
                             std::string& calib_page) {
  std::string vref_name = "TOT_VREF";
  std::string calib_name{preCC ? "CALIB_2V5" : "CALIB"};
  int calib_value{100000};
  double tot_eff{0};
  int i_ch = channel % 36;  // 0–35
  int n_links = 2;
  if constexpr (std::is_same_v<EventPacket,
                               pflib::packing::MultiSampleECONDEventPacket>) {
    n_links = determine_n_links(tgt);
  }
  // Vectors for storing tot_eff and calib for the current param_point
  std::vector<double> tot_eff_list;
  std::vector<int> calib_list = {0, 4095};  // min and max
  // tolerance for checking distance between tot_eff and 0.5
  double tol{0.1};
  int count{2};
  int max_its = 25;
  int vref_value{0};

  std::vector<EventPacket> buffer;
  DecodeAndWriteToCSV<EventPacket> writer{
      fname,
      [&](std::ofstream& f) {
        nlohmann::json header;
        header["channel"] = channel;
        header["highrange"] = highrange;
        header["preCC"] = preCC;
        f << std::boolalpha << "# " << header << '\n' << "time,";
        f << vref_page << '.' << vref_name << ',' << calib_page << '.'
          << calib_name << ',';
        f << pflib::packing::Sample::to_csv_header << '\n';
      },
      [&](std::ofstream& f, const EventPacket& ep) {
        f << time << ',';
        f << vref_value << ',';
        f << calib_value << ',';

        if constexpr (std::is_same_v<EventPacket,
                                     pflib::packing::SingleROCEventPacket>) {
          ep.channel(channel).to_csv(f);
        } else if constexpr (std::is_same_v<
                                 EventPacket,
                                 pflib::packing::MultiSampleECONDEventPacket>) {
          ep.samples[ep.i_soi].channel(link, i_ch).to_csv(f);
        }

        f << '\n';
        buffer.push_back(ep);
      },
      n_links};

  tgt->setup_run(1 /* dummy - not stored */, pftool::state.daq_format_mode,
                 1 /* dummy */);
  for (vref_value = vref_lower; vref_value <= vref_upper;
       vref_value += nsteps) {
    // reset for every iteration
    tot_eff_list.clear();
    calib_list = {0, 4095};
    calib_value = 100000;
    tot_eff = 0;
    auto vref_test_param =
        roc.testParameters().add(vref_page, vref_name, vref_value).apply();
    pflib_log(info) << vref_name << " = " << vref_value;
    while (true) {
      if (search) {
        // BINARY SEARCH
        if (!tot_eff_list.empty()) {
          if (tot_eff_list.back() > 0.5) {
            calib_value = std::abs(calib_list.back() -
                                   calib_list[calib_list.size() - 2]) /
                              2 +
                          calib_list[calib_list.size() - count];
            count++;
          } else {
            calib_value = std::abs((calib_list[calib_list.size() - 2] -
                                    calib_list.back())) /
                              2 +
                          calib_list.back();
            count = 2;
          }
        } else {
          calib_value = calib_list.front();
        }
      } else {
        // BISECTIONAL SEARCH
        if (!tot_eff_list.empty()) {
          if (tot_eff_list.back() > 0.5) {
            calib_list.back() = calib_value;
          } else {
            calib_list.front() = calib_value;
          }
          calib_value =
              (calib_list.back() - calib_list.front()) / 2 + calib_list.front();
        } else {
          calib_value = (calib_list.back() - calib_list.front()) / 2;
        }
      }
      auto calib_test_param =
          roc.testParameters().add(calib_page, calib_name, calib_value).apply();
      usleep(10);  // make sure parameters are applied

      buffer.clear();

      // daq run
      daq_run(tgt, "CHARGE", writer, nevents, pftool::state.daq_rate);

      std::vector<double> tot_list;
      int tot = 0;

      for (EventPacket ep : buffer) {
        if constexpr (std::is_same_v<EventPacket,
                                     pflib::packing::SingleROCEventPacket>) {
          tot = ep.channel(channel).tot();
        } else if constexpr (std::is_same_v<
                                 EventPacket,
                                 pflib::packing::MultiSampleECONDEventPacket>) {
          tot = ep.samples[ep.i_soi].channel(link, i_ch).tot();
        }

        if (tot > 0) {
          tot_list.push_back(tot);
        }
      }

      // Calculate tot_eff
      tot_eff = static_cast<double>(tot_list.size()) / nevents;

      if (search) {
        // BINARY SEARCH
        if (std::abs(calib_value - calib_list.back()) < tol) {
          pflib_log(info) << "Final calib is : " << calib_value
                          << " with tot_eff = " << tot_eff;
          break;
        }
      } else {
        // BISECTIONAL SEARCH
        if (std::abs(tot_eff - 0.5) < tol) {
          pflib_log(info) << "Final calib is : " << calib_value
                          << " with tot_eff = " << tot_eff;
          break;
        }
        if (calib_value == 0 || calib_value == 4094) {
          pflib_log(info) << "No v_t50 was found!";
          break;
        }
      }

      tot_eff_list.push_back(tot_eff);
      if (search) calib_list.push_back(calib_value);

      // Sometimes we can't hone in close enough to 0.5 because calib are whole
      // ints
      if (tot_eff_list.size() > max_its) {
        pflib_log(info) << "Ended after " << max_its << " iterations!" << '\n'
                        << "Final calib is : " << calib_value
                        << " with tot_eff = " << tot_eff;
        break;
      }
    }
  }
}

void vt50_scan(Target* tgt) {
  int nevents = pftool::readline_int(
      "How many events per time point? Remember that the tot efficiency's "
      "granularity depends on this number. ",
      20);
  bool preCC = pftool::readline_bool("Use pre-CC charge injection? ", false);
  bool highrange = false;
  if (!preCC)
    highrange =
        pftool::readline_bool("Use highrange (Y) or lowrange (N)? ", false);
  bool search = pftool::readline_bool(
      "Use binary (Y) or bisectional search (N)? ", false);
  int channel = pftool::readline_int("Channel to pulse into? ", 61);
  int toa_threshold = pftool::readline_int("Value for TOA threshold: ", 250);
  int vref_lower =
      pftool::readline_int("Smallest tot threshold value (min = 0): ", 300);
  int vref_upper =
      pftool::readline_int("Largest tot threshold value (max = 4095): ", 600);
  int nsteps = pftool::readline_int("Number of steps between tot values: ", 10);
  std::string fname = pftool::readline_path("vt50-scan", ".csv");
  auto roc{tgt->roc(pftool::state.iroc)};
  auto channel_page = pflib::utility::string_format("CH_%d", channel);
  int link = (channel / 36);
  auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
  auto test_param_handle =
      roc.testParameters()
          .add(refvol_page, "INTCTEST", 1)
          .add(refvol_page, "TOA_VREF", toa_threshold)
          .add(refvol_page, "CHOICE_CINJ", (highrange && !preCC) ? 1 : 0)
          .add(channel_page, "HIGHRANGE", (highrange || preCC) ? 1 : 0)
          .add(channel_page, "LOWRANGE",
               preCC       ? 0
               : highrange ? 0
                           : 1)
          .apply();

  std::string vref_page = refvol_page;
  std::string calib_page = refvol_page;

  if (pftool::state.daq_format_mode == Target::DaqFormat::SIMPLEROC) {
    vt50_scan_writer<pflib::packing::SingleROCEventPacket>(
        tgt, roc, nevents, preCC, highrange, search, channel, toa_threshold,
        vref_lower, vref_upper, nsteps, fname, link, vref_page, calib_page);
  } else if (pftool::state.daq_format_mode ==
             Target::DaqFormat::ECOND_SW_HEADERS) {
    vt50_scan_writer<pflib::packing::MultiSampleECONDEventPacket>(
        tgt, roc, nevents, preCC, highrange, search, channel, toa_threshold,
        vref_lower, vref_upper, nsteps, fname, link, vref_page, calib_page);
  }
}
