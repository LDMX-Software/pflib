#include "vt50_scan.h"

#include "pflib/DecodeAndWrite.h"
#include "pflib/utility/json.h"
#include "pflib/utility/string_format.h"

ENABLE_LOGGING();

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
  auto roc{tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version())};
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
  std::string vref_name = "TOT_VREF";
  std::string calib_name{preCC ? "CALIB_2V5" : "CALIB"};
  int calib_value{100000};
  double tot_eff{0};

  // Vectors for storing tot_eff and calib for the current param_point
  std::vector<double> tot_eff_list;
  std::vector<int> calib_list = {0, 4095};  // min and max
  // tolerance for checking distance between tot_eff and 0.5
  double tol{0.1};
  int count{2};
  int max_its = 25;
  int vref_value{0};

  std::vector<pflib::packing::SingleROCEventPacket> buffer;
  pflib::DecodeAndWriteToCSV writer{
      fname,
      [&](std::ofstream& f) {
        boost::json::object header;
        header["channel"] = channel;
        header["highrange"] = highrange;
        header["preCC"] = preCC;
        f << std::boolalpha << "# " << boost::json::serialize(header) << '\n'
          << "time,";
        f << vref_page << '.' << vref_name << ',' << calib_page << '.'
          << calib_name << ',';
        f << pflib::packing::Sample::to_csv_header << '\n';
      },
      [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
        f << time << ',';
        f << vref_value << ',';
        f << calib_value << ',';
        ep.channel(channel).to_csv(f);
        f << '\n';
        buffer.push_back(ep);
      }};

  tgt->setup_run(1 /* dummy - not stored */, Target::DaqFormat::SIMPLEROC,
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
      tgt->daq_run("CHARGE", writer, nevents, pftool::state.daq_rate);

      std::vector<double> tot_list;
      for (pflib::packing::SingleROCEventPacket ep : buffer) {
        auto tot = ep.channel(channel).tot();
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
