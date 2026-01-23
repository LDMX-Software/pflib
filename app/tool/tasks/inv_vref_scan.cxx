#include "inv_vref_scan.h"

#include <nlohmann/json.hpp>

#include "../daq_run.h"
#include "../econ_links.h"

ENABLE_LOGGING();

template <typename T>
double median(std::vector<T>& val) {
    size_t n = val.size();
    if (n == 0) return 0.0;

    auto mid = val.begin() + n / 2;
    std::nth_element(val.begin(), mid, val.end());

    if (n % 2 == 1)
        return *mid;

    auto mid2 = std::max_element(val.begin(), mid);
    return 0.5 * (*mid + *mid2);
}

void DataFitter::sort_and_append(std::vector<int>& inv_vrefs, std::vector<int>& pedestals) {
  static auto the_log_{::pflib::logging::get("inv_vref_scan")};
  // We ignore first and last elements since they miss derivs
  struct DerivPoint {
    int i;
    double LH;
    double RH;
  };
  std::vector<DerivPoint> slope_points;

  double flat_threshold = 0.05;
  std::vector<double> LH_derivs;
  std::vector<double> RH_derivs;
  for (int i = 1; i < inv_vrefs.size()-1; i++) {
    double LH = double(pedestals[i] - pedestals[i-1]) / (inv_vrefs[i] - inv_vrefs[i-1]);
    double RH = double(pedestals[i] - pedestals[i+1]) / (inv_vrefs[i] - inv_vrefs[i+1]);

    // Threshold check. CMS uses 0.05
    if (std::abs(LH) <= flat_threshold || std::abs(RH) <= flat_threshold) { // flat regime
      nonlinear_.push_back({inv_vrefs[i], pedestals[i], LH, RH});
    } else { // we're in a linear regime or there's an outlier
      slope_points.push_back({i, LH, RH});
      LH_derivs.push_back(LH);
      RH_derivs.push_back(RH);
    }
  }
  // Now we get the slopey region, removing outliers by using the median
  double LH_median = median(LH_derivs);
  double RH_median = median(RH_derivs);
  
  for (const auto& p : slope_points) {
    if (std::abs(p.LH - LH_median) < 0.3 * LH_median) { // Linear regime. implement RH_median here as well? 
      linear_.push_back({inv_vrefs[p.i], pedestals[p.i], p.LH, p.RH});
    }
  }

  // Calculate the median intercept as a sanity check

  std::vector<double> intercepts;
  for (int i = 1; i < linear_.size()-1; i++) {
    double m = linear_[i].y_ - linear_[i].LH_ * linear_[i].x_;
    intercepts.push_back(m);
  }
  double median_intercept = median(intercepts);
  pflib_log(info) << "The median intercept is " << median_intercept;

}



// helper function to facilitate EventPacket dependent behaviour
template <class EventPacket>
static void inv_vref_scan_writer(Target* tgt, pflib::ROC& roc, size_t nevents,
                                 std::string& output_filepath,
                                 std::array<int, 2>& channels, int& inv_vref) {
  int i_link = 0;
  int i_ch = 0;  // 0–35
  int n_links = 2;
  if constexpr (std::is_same_v<EventPacket,
                               pflib::packing::MultiSampleECONDEventPacket>) {
    n_links = determine_n_links(tgt);
  }
  //DecodeAndWriteToCSV<EventPacket> writer{
  //    output_filepath,
  //    [&](std::ofstream& f) {
  //      nlohmann::json header;
  //      header["scan_type"] = "CH_#.INV_VREF sweep";
  //      header["trigger"] = "PEDESTAL";
  //      header["nevents_per_point"] = nevents;
  //      f << "# " << header << "\n"
  //        << "INV_VREF";
  //      for (int ch : channels) {
  //        f << ',' << ch;
  //      }
  //      f << '\n';
  //    },
  //    [&](std::ofstream& f, const EventPacket& ep) {
  //      f << inv_vref;
  //      for (int ch : channels) {
  //        i_link = (ch / 36);
  //        i_ch = ch % 36;
  //        if constexpr (std::is_same_v<EventPacket,
  //                                     pflib::packing::SingleROCEventPacket>) {
  //          f << ',' << ep.channel(ch).adc();
  //        } else if constexpr (
  //            std::is_same_v<EventPacket,
  //                           pflib::packing::MultiSampleECONDEventPacket>) {
  //          f << ',' << ep.samples[ep.i_soi].channel(i_link, i_ch).adc();
  //        }
  //      }
  //      f << '\n';
  //    },
  //    n_links};
  
  DecodeAndBuffer<EventPacket> buffer{1, 2};

  tgt->setup_run(1 /* dummy - not stored */, pftool::state.daq_format_mode,
                 1 /* dummy */);

  // increment inv_vref in increments of 20. 10 bit value but only scanning to
  // 600
  int range = 10; // defines range of inv_vref values (x-range)
  std::vector<int> pedestals_l0;
  std::vector<int> pedestals_l1;
  std::vector<int> inv_vrefs;

  for (inv_vref = 300; inv_vref <= 310; inv_vref += 1) {
    pflib_log(info) << "Running INV_VREF = " << inv_vref;
    // set inv_vref simultaneously for both links
    auto test_param = roc.testParameters()
                          .add("REFERENCEVOLTAGE_0", "INV_VREF", inv_vref)
                          .add("REFERENCEVOLTAGE_1", "INV_VREF", inv_vref)
                          .apply();
    // store current scan state in header for writer access
    daq_run(tgt, "PEDESTAL", buffer, nevents, pftool::state.daq_rate);

    auto data = buffer.get_buffer();

    std::vector<int> adcs_l0;
    std::vector<int> adcs_l1;

    for (std::size_t i{0}; i < data.size(); i++) {
      if constexpr (std::is_same_v<
                        EventPacket,
                        pflib::packing::MultiSampleECONDEventPacket>) {
        adcs_l0.push_back(
            data[i].samples[data[i].i_soi].channel(i_link, channels[0]).adc());
        adcs_l1.push_back(
            data[i].samples[data[i].i_soi].channel(i_link, channels[1]).adc());
      } else if constexpr (std::is_same_v<
                               EventPacket,
                               pflib::packing::SingleROCEventPacket>) {
        adcs_l0.push_back(data[i].channel(channels[0]).adc());
        adcs_l1.push_back(data[i].channel(channels[1]).adc());
      } else {
        PFEXCEPTION_RAISE("BadConf",
                          "Unable to get adc for the cofigured format");
        }
      }
    pedestals_l0.push_back(median(adcs_l0)); pedestals_l1.push_back(median(adcs_l1));
    inv_vrefs.push_back(inv_vref);
    }
  // sort data and fit
  DataFitter fitter_l0;
  DataFitter fitter_l1;
  fitter_l0.sort_and_append(inv_vrefs, pedestals_l0);
  fitter_l1.sort_and_append(inv_vrefs, pedestals_l1);
  
}

void inv_vref_scan(Target* tgt) {
  int nevents = pftool::readline_int("Number of events per point: ", 1);
  int inv_vref = 0;

  std::string output_filepath = pftool::readline_path("inv_vref_scan", ".csv");
  int ch_link0 = pftool::readline_int("Channel to scan on link 0", 17);
  int ch_link1 = pftool::readline_int("Channel to scan on link 1", 51);
  std::array<int, 2> channels = {ch_link0, ch_link1};

  auto roc = tgt->roc(pftool::state.iroc);

  if (pftool::state.daq_format_mode == Target::DaqFormat::SIMPLEROC) {
    inv_vref_scan_writer<pflib::packing::SingleROCEventPacket>(
        tgt, roc, nevents, output_filepath, channels, inv_vref);
  } else if (pftool::state.daq_format_mode ==
             Target::DaqFormat::ECOND_SW_HEADERS) {
    inv_vref_scan_writer<pflib::packing::MultiSampleECONDEventPacket>(
        tgt, roc, nevents, output_filepath, channels, inv_vref);
  }

  // DecodeAndWriteToCSV writer{
  //     output_filepath,
  //     [&](std::ofstream& f) {
  //       nlohmann::json header;
  //       header["scan_type"] = "CH_#.INV_VREF sweep";
  //       header["trigger"] = "PEDESTAL";
  //       header["nevents_per_point"] = nevents;
  //       f << "# " << header << "\n"
  //         << "INV_VREF";
  //       for (int ch : channels) {
  //         f << ',' << ch;
  //       }
  //       f << '\n';
  //     },
  //     [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
  //       f << inv_vref;
  //       for (int ch : channels) {
  //         f << ',' << ep.channel(ch).adc();
  //       }
  //       f << '\n';
  //     }};

  // tgt->setup_run(1 /* dummy */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);

  // // increment inv_vref in increments of 20. 10 bit value but only scanning
  // to
  // // 600
  // for (inv_vref = 0; inv_vref <= 600; inv_vref += 20) {
  //   pflib_log(info) << "Running INV_VREF = " << inv_vref;
  //   // set inv_vref simultaneously for both links
  //   auto test_param = roc.testParameters()
  //                         .add("REFERENCEVOLTAGE_0", "INV_VREF", inv_vref)
  //                         .add("REFERENCEVOLTAGE_1", "INV_VREF", inv_vref)
  //                         .apply();
  //   // store current scan state in header for writer access
  //   daq_run(tgt, "PEDESTAL", writer, nevents, pftool::state.daq_rate);
  // }
}
