#include "trim_toa_scan.h"
#include "../daq_run.h"
#include "../tasks/trim_toa_scan.h"
#include "get_toa_efficiencies.h"
#include "pflib/utility/median.h"
#include "pflib/utility/string_format.h"
#include <numeric>
#include <vector>
#include <fstream>

/**
 * @param[in] x_vals list of x-coordinate samples
 * @param[in] y_vals list of y-coordinate samples
 * @return 2-tuple of the form (slope, intercept)
 */

std::tuple<double, double> linear_regression(
    const std::vector<double>& x_vals, const std::vector<double>& y_vals) {
  if (x_vals.size() != y_vals.size()) {
    throw std::invalid_argument("x_vals and y_vals must be the same size."); //applies a check to see if the data that it is analyzing is of a suitable size
  }
int size = x_vals.size();

double x_sum = 0.0;
double y_sum = 0.0;

for (std::size_t i = 0; i < size; i++){
x_sum += x_vals[i];
y_sum += y_vals[i];
}

  double x_mean = x_sum / size;
  double y_mean = y_sum / size;
  double s_x = 0.0;
  double s_y = 0.0;
  double s_xy = 0.0;

  for (std::size_t i = 0; i < size; i++){
    s_x += (x_vals[i] - x_mean) * (x_vals[i] - x_mean);
    s_y += (y_vals[i] - y_mean) * (y_vals[i] - y_mean);
    s_xy += (x_vals[i] - x_mean) * (y_vals[i] - y_mean);
  }

  double slope = s_xy / s_x;
  double intercept = y_mean - slope * x_mean;

  return std::make_tuple(slope, intercept);
}

/**
 * Calculate the TRIM_TOA for each channel that best aligns all of them
 * to a common threshold voltage, charge injection pulse (calib).
 */

namespace pflib::algorithm {
std::map<int, std::map<std::string, std::map<std::string, uint64_t>>>trim_toa_scan(
    Target* tgt)
{static auto the_log_{::pflib::logging::get("trim_toa_scan")};

  /**
   * Charge injection scan (100 samples) while varying TRIM_TOA.
   * Purpose is to align TRIM_TOA for each channel.
   * Calculates TOA efficiency while looking at charge injection data.
   * Then uses A NORMAL Linear Regression to calculate the aligned
   * TRIM_TOA value for each channel to match a common "calib" value.
   *
   * @note Reduce the sample size (ex: 100 to 10) to decrease the scan time.
   */

  static const std::size_t n_events = 10;
  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);
  tgt->fc().fc_setup_calib(17); //Sets the bunch crossing value, should have the optimal value be the same for both links on a ROC.
  // trim_toa is a channel-wise parameter (1 value per channel)

  int calib_step = 4;
  int trim_toa_step = 2;
  int trim_toa_max = 20;
  int calib_max = 300;
  int calib_min = 0;

  //Has been changed based on the size of the scan I am running, edit in the above variables
  auto final_data = std::vector<std::vector<std::vector<std::vector<double>>>>(tgt -> nrocs(), std::vector<std::vector<std::vector<double>>>(((calib_max-calib_min)/calib_step) , std::vector<std::vector<double>>((trim_toa_max/trim_toa_step), std::vector<double>(72, 0))));

  // working in buffer, not in writer, changed to not start trim_toa at 0 but instead at 12
  DecodeAndBuffer buffer{n_events, tgt -> nrocs() * 2};

  std::map<int, int> roc_index; //Added to stop final_data from being out of bounds of the nrocs length
  int count = 0;
  for (int i_roc : tgt->roc_ids()) {
    roc_index[i_roc] = count;
    count += 1;
  }

  // loop over trim_toa, over the full range of 0 to 20  with a stepsize of trim_toa_step
  // loop over the ROCs and each of the channels
  // loop over calib, stepsize of calib_step over a range of CALIB = 0 to 200
const pflib::packing::SingleECONDRocErxMapping& mapping = tgt->getRocErxMapping();
std::map<std::string, std::map<std::string, uint64_t>> charge_active;
for (int i_link{0}; i_link < 2; i_link++) {
      std::string refvol_page{pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link)};
      charge_active[refvol_page]["INTCTEST"] = 1;
      charge_active[refvol_page]["CHOICE_CINJ"] = 1;
      }
auto charge_params = tgt->tempApplyAllROCs(charge_active);
//pflib_log(info) << "Applied charge params";

  for (int ch{0}; ch < 72; ch++) {
    pflib_log(info) << "on channel = " << ch ;
    // set TRIM_TOA for each channel
    std::map<std::string, std::map<std::string, uint64_t>> param_ch;
    std::string ch_str{"CH_" + std::to_string(ch)};
    param_ch[ch_str]["HIGHRANGE"] = 1;
    param_ch[ch_str]["LOWRANGE"] = 0;
    auto param_apply = tgt->tempApplyAllROCs(param_ch);

    for (int trim_toa{0}; trim_toa < trim_toa_max; trim_toa += trim_toa_step) {
      std::map<int, bool> good_trim_val;
      for (int i_roc : tgt->roc_ids()) {
        good_trim_val[i_roc] = false;
      }
      auto all_good = [&]() {
        for (const auto& [i_roc, val] : good_trim_val) {
           if (!val) return false;
        }
          return true;
        };

      std::map<std::string, std::map<std::string, uint64_t>> trim;
      trim[ch_str]["TRIM_TOA"] = trim_toa;
      auto trim_apply = tgt->tempApplyAllROCs(trim);

      pflib_log(info) << "testing trim_toa = " << trim_toa;
      for (int calib = calib_min; calib < calib_max && !all_good(); calib += calib_step ) {
        std::map<std::string, std::map<std::string, uint64_t>> parameters;
        //pflib_log(info) << "Before Applied CALIB";
        for (int i_link{0}; i_link < 2; i_link++) {
          std::string refvol_page{pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link)};
          parameters[refvol_page]["CALIB"] = calib;
          }
        auto test_params = tgt->tempApplyAllROCs(parameters);
        //pflib_log(info) << "Applied CALIB = " << calib << " on both links";
        usleep(10);
        daq_run(tgt, "CHARGE", buffer, n_events, pftool::state.daq_rate);
        auto data = buffer.get_buffer();
        int length = data.size();
        for (int i_roc : tgt->roc_ids()) {
        int toa_count = 0;
        auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, ch);

        for (std::size_t i = 0; i < length; i++){
          double toa = data[i].soi().channel(i_erx, i_ch).toa();
          if (toa > 0){
            ++toa_count;
            }
          }

        double percent = ((toa_count * 1.0) / length);
      //pflib_log(info) << " percent = " << percent << "\n";
      //auto efficiencies = get_toa_efficiencies(i_roc, mapping, buffer.get_buffer()) ;
      //pflib_log(info) << "got channel efficiencies, storing now" ;
      // need to divide by 4 because index is value/4 from final_data, initialization and for loop
        final_data[roc_index[i_roc]][(calib - calib_min)/ calib_step][trim_toa / trim_toa_step][ch] = percent; //sets data to be called later as threshold value

        if (percent > 0.95) {
          std::cout << "non-negligable efficiency: ch=" << ch << " calib=" << calib << " ROC=" << i_roc << " trim_toa=" << trim_toa << " eff=" << percent << std::endl; //print with useful data
          good_trim_val[i_roc] = true;
       }
      }
     }
    }

    std::map<std::string, std::map<std::string, uint64_t>> parameters2;
    parameters2[ch_str]["HIGHRANGE"] = 0;
    auto test_params2 = tgt->tempApplyAllROCs(parameters2);
   }

pflib_log(info) << "sample collections done, deducing settings";

//saving the efficiencies to a csv file to have to run this files less times
std::ofstream csv_file("toa_efficiencies.csv");
for (int i_roc : tgt->roc_ids()) {
  for (int trim_toa{0}; trim_toa < trim_toa_max ; trim_toa += trim_toa_step) {
    for (int ch{0}; ch < 72; ch++) {
      for (int calib{calib_min}; calib < calib_max ; calib += calib_step) {
        // divide by the proper stepsize for indexing
        double efficiency = final_data[roc_index[i_roc]][(calib - calib_min) / calib_step][trim_toa / trim_toa_step][ch];
        csv_file << i_roc << "," << ch << "," << trim_toa << "," << calib << "," << efficiency << "\n";
      }
    }
  }
}

pflib_log(info) << "Saved data file successfully";


//Saving the efficiencies data so this does not need to be redone over and over again

  /**
   * Now that we have the data, we need to analyze it.
   *
   * We'll be looking for the turn-on (threshold) points for each channel
   * at each trim_toa value. The turn-on (threshold) point is the first
   * point where toa_efficiency goes from 0 to non-zero. The toa_efficiency
   * a given trim_toa/calib/channel combination.
   *
   * We'll be using a normal Linear Regression because it's less sensitive to
   * outliers, since sometimes changing the trim_toa causes the threshold
   * (turn-on) points to "wrap around".
   */

  // "threshold_points" is a 2D vector.
  // Column 1 is channel index, Column 2 is calib, Column 3 is trim_toa.
  std::vector<std::vector<int>> threshold_points;
  //std::vector<int> calib_vals;
  // Each channel has multiple threshold points, but we don't know how many yet

pflib_log(trace) << "assigning threshold values";
for (int i_roc : tgt->roc_ids()) {
  for (int trim_toa{0}; trim_toa < trim_toa_max; trim_toa += trim_toa_step) {
    for (int ch{0}; ch < 72; ch++) {
      for (int calib{calib_min}; calib < calib_max ; calib += calib_step) {
        // divide by the proper stepsize to convert value to index
        if (final_data[roc_index[i_roc]][(calib - calib_min) / calib_step][trim_toa / trim_toa_step][ch]> 0.9) {
          threshold_points.push_back({ch, calib, trim_toa, i_roc});
          //calib_vals.push_back(calib);
          break;
        }
      }
    }
  }
}

pflib_log(trace) << "Completed Assigning Threshold Values";
//int calib_med = pflib::utility::median(calib_vals) ;
int calib_tgt = 75 ;  //base this value around what your target calib value is for the highrange


// get vector of data points for each channel.
std::map<int, std::array<int, 72>> target;
for (int i_roc : tgt-> roc_ids()) {
  std::vector<uint64_t> avg_toa_vals; //For the averaging for the channels that are missing data, it is designed to have them be calculated per
                                      //ROC, as I have found that the behaviors of the data for each ROC seems to be unique to each chip
  for (int ch{0}; ch < 72; ch++) {
    std::vector<double> calib;
    std::vector<double> trim_toa;

    for (const auto& row : threshold_points) {
      if (row[3] == i_roc){
      if (row[0] == ch) {
        calib.push_back(row[1]);
        trim_toa.push_back(row[2]);
       }
      }
    }
    if (calib.size() < 2) { //Code made to remove the data that was too insignificant
    std::cout << "Skipping channel " << ch << " due to insufficient points, setting to be middle value (" << calib.size() << ")" << std::endl;
    //Need to have this averaging because some of the channels simply dont capture any data at all (some of them are dead, but those dont factor into calculating this average)
    target[roc_index[i_roc]][ch] = static_cast<int>(-1); //Identify these dead channels as 0, which is a value none of the working channels produce
    continue;
    }
    //std::cout << "x_vals size: " << calib.size() << ", y_vals size: " << trim_toa.size() << std::endl;
    auto [slope, intercept] = linear_regression(calib, trim_toa);  //applies the linear regression to your data
    pflib_log(trace) << "Slope value was found to be = " << slope << "\n";
    int optimal_trim_val_round =  static_cast<int>(std::round(calib_tgt * slope + intercept));
    pflib_log(trace) << "Appended optimal_trim_val = " << optimal_trim_val_round << "\n";
    int optimal_trim_val_clam = std::clamp(optimal_trim_val_round, 0, 63);
    int optimal_trim_val =  static_cast<uint64_t>(std::round(optimal_trim_val_clam)); //apply settings to make the optimal trim more reasonable
    target[roc_index[i_roc]][ch] = static_cast<int>(optimal_trim_val);
    avg_toa_vals.push_back(optimal_trim_val); //append data to be averaged
 }

int sum = std::reduce(avg_toa_vals.begin(), avg_toa_vals.end(), 0);
int average = std::round(std::reduce(avg_toa_vals.begin(), avg_toa_vals.end(), 0.0) / avg_toa_vals.size());

for (int ch = 0; ch < 72; ch++) {
    if (target[roc_index[i_roc]][ch] == -1) {
        target[roc_index[i_roc]][ch] = average;
    }
    }

pflib_log(trace) << "did the averaging and found it to be:" << average;
pflib_log(info) << "Completed Regression";
}

pflib_log(trace) << "writing TRIM_TOA settings";

std::map<int, std::map<std::string, std::map<std::string, uint64_t>>> settings;
 for (int i_roc : tgt-> roc_ids()) {
  for (int ch{0}; ch < 72; ch++) {
    std::string page{pflib::utility::string_format("CH_%d", ch)};
    settings[i_roc][page]["TRIM_TOA"] = target[roc_index[i_roc]][ch];
}
}
  return settings;
}
}  // namespace pflib::algorithm
