#include "inv_vref_align.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <type_traits>

#include "../daq_run.h"
#include "../econ_links.h"
#include "../tasks/level_pedestals.h"  // for pftool::state.daq_format_mode, like existing code
#include "pflib/utility/median.h"

namespace pflib::algorithm {

template <class EventPacket>
static int get_adc(const EventPacket& p, int ch) {
  if constexpr (std::is_same_v<EventPacket, pflib::packing::SingleROCEventPacket>) {
    return p.channel(ch).adc();
  } else if constexpr (std::is_same_v<EventPacket, pflib::packing::MultiSampleECONDEventPacket>) {
    int i_link = ch / 36;
    int i_ch = ch % 36;
    return p.samples[p.i_soi].channel(i_link, i_ch).adc();
  } else {
    static_assert(sizeof(EventPacket) == 0, "Unsupported packet type in get_adc()");
  }
}

template <class EventPacket>
static InvVrefScanResult inv_vref_scan_data_impl(pflib::Target* tgt, pflib::ROC& roc,
                                                 int nevents, int ch0, int ch1,
                                                 int step, int max_vref) {
  int n_links = 2;
  if constexpr (std::is_same_v<EventPacket, pflib::packing::MultiSampleECONDEventPacket>) {
    n_links = determine_n_links(tgt);
  }

  DecodeAndBuffer<EventPacket> buffer{static_cast<std::size_t>(nevents), n_links};

  // Use the DAQ format selected in pftool (consistent with existing tasks/algorithms)
  tgt->setup_run(1 /*dummy*/, pftool::state.daq_format_mode, 1 /*dummy*/);

  InvVrefScanResult out;
  out.ch0 = ch0;
  out.ch1 = ch1;

  std::vector<int> tmp;
  tmp.resize(static_cast<std::size_t>(nevents));

  for (int inv = 0; inv <= max_vref; inv += step) {
    // set inv_vref for both links simultaneously
    roc.testParameters()
        .add("REFERENCEVOLTAGE_0", "INV_VREF", inv)
        .add("REFERENCEVOLTAGE_1", "INV_VREF", inv)
        .apply();

    // collect nevents
    daq_run(tgt, "PEDESTAL", buffer, static_cast<std::size_t>(nevents), pftool::state.daq_rate);

    const auto& data = buffer.get_buffer();
    if (data.empty()) {
      throw std::runtime_error("inv_vref_scan_data: empty DAQ buffer");
    }

    // median ADC for ch0
    for (std::size_t i = 0; i < data.size(); ++i) tmp[i] = get_adc<EventPacket>(data[i], ch0);
    int med0 = pflib::utility::median(tmp);

    // median ADC for ch1
    for (std::size_t i = 0; i < data.size(); ++i) tmp[i] = get_adc<EventPacket>(data[i], ch1);
    int med1 = pflib::utility::median(tmp);

    out.inv_vref.push_back(inv);
    out.adc0.push_back(med0);
    out.adc1.push_back(med1);
  }

  return out;
}

InvVrefScanResult inv_vref_scan_data(pflib::Target* tgt, pflib::ROC& roc,
                                     int nevents, int ch0, int ch1,
                                     int step, int max_vref) {
  if (pftool::state.daq_format_mode == Target::DaqFormat::SIMPLEROC) {
    return inv_vref_scan_data_impl<pflib::packing::SingleROCEventPacket>(
        tgt, roc, nevents, ch0, ch1, step, max_vref);
  }
  if (pftool::state.daq_format_mode == Target::DaqFormat::ECOND_SW_HEADERS) {
    return inv_vref_scan_data_impl<pflib::packing::MultiSampleECONDEventPacket>(
        tgt, roc, nevents, ch0, ch1, step, max_vref);
  }

  throw std::runtime_error("inv_vref_scan_data: unsupported DAQ format mode");
}

static void linear_fit_filtered_10_80(const std::vector<int>& x,
                                      const std::vector<int>& y,
                                      double& slope, double& offset) {
  if (x.size() != y.size() || x.size() < 3) {
    throw std::runtime_error("inv_vref_align: not enough points to fit");
  }

  auto [min_it, max_it] = std::minmax_element(y.begin(), y.end());
  const double ymin = static_cast<double>(*min_it);
  const double ymax = static_cast<double>(*max_it);
  const double lo = ymin + 0.1 * (ymax - ymin);
  const double hi = ymin + 0.8 * (ymax - ymin);

  // accumulate on filtered region
  double Sx = 0, Sy = 0, Sxx = 0, Sxy = 0;
  int n = 0;
  for (std::size_t i = 0; i < x.size(); ++i) {
    const double yi = static_cast<double>(y[i]);
    if (yi <= lo || yi >= hi) continue;
    const double xi = static_cast<double>(x[i]);
    Sx += xi;
    Sy += yi;
    Sxx += xi * xi;
    Sxy += xi * yi;
    ++n;
  }

  if (n < 2) {
    throw std::runtime_error("inv_vref_align: insufficient points in 10%-80% linear regime");
  }

  const double denom = (n * Sxx - Sx * Sx);
  if (std::abs(denom) < 1e-12) {
    throw std::runtime_error("inv_vref_align: degenerate fit (denom ~ 0)");
  }

  slope  = (n * Sxy - Sx * Sy) / denom;
  offset = (Sy - slope * Sx) / n;  // y = slope*x + offset
}

std::map<std::string, std::map<std::string, uint64_t>> inv_vref_align(
    const InvVrefScanResult& scan, int target_adc) {
  double s0 = 0, b0 = 0;
  double s1 = 0, b1 = 0;

  linear_fit_filtered_10_80(scan.inv_vref, scan.adc0, s0, b0);
  linear_fit_filtered_10_80(scan.inv_vref, scan.adc1, s1, b1);

  const int inv0 = static_cast<int>(std::llround((target_adc - b0) / s0));
  const int inv1 = static_cast<int>(std::llround((target_adc - b1) / s1));

  std::map<std::string, std::map<std::string, uint64_t>> settings;
  settings["REFERENCEVOLTAGE_0"]["INV_VREF"] = static_cast<uint64_t>(std::clamp(inv0, 0, 1023));
  settings["REFERENCEVOLTAGE_1"]["INV_VREF"] = static_cast<uint64_t>(std::clamp(inv1, 0, 1023));
  return settings;
}

}  // namespace pflib::algorithm
