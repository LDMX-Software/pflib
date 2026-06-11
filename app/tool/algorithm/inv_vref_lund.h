#pragma once

#include "../daq_run.h"
#include "pflib/Target.h"

/**
 * @namespace pflib::algorithm
 * housing of higher-level methods for repeatable tasks
 */
namespace pflib::algorithm {

/**
 *
 * Data class that stores information about each point in a 1d space.
 * sort_and_append() sorts the data according to inv_vref selections that locate
 * a linear region in the INV_VREF parameter space, and then appends to each
 * Point fit() fits the linear region according to a standard linear fit.
 *
 */
class DataFitter {
 public:
  DataFitter();

  // Member functions

  void sort_and_append(std::vector<int>& inv_vrefs, std::vector<int>& pedestals,
                       std::vector<double>& stds, int& step);
  int fit(int target);

  // Member variables

  struct Point {
    int x_;
    int y_;
    double LH_;
    double RH_;
  };
  std::vector<Point> linear_;
  std::vector<Point> nonlinear_;
  double LH_median_;
  double LH_std_median_;
  double RH_median_;
};

/**
 * Collect inv_vref data for the constant input noinv_vref parameter.
 * Loops over the whole inv_vref range.
 */
void get_param(Target* tgt, DecodeAndBuffer& buffer, const std::size_t& nevents, int& step,
		std::map<int, std::vector<int>>& pedestals_l0,
		std::map<int, std::vector<double>>& stds_l0,
		std::map<int, std::vector<int>>& pedestals_l1,
		std::map<int, std::vector<double>>& stds_l1,
		std::map<int, std::vector<int>>& inv_vrefs,
		int& noinv_vref);

/**
 * Find the inv_vref and noinv_vref parameters.
 *
 * @param[in] tgt pointer to Target to interact with
 *
 * @note Only functional for single-ROC targets
 */
std::map<int, std::map<std::string, std::map<std::string, uint64_t>>> inv_vref_lund(
    Target* tgt);

}  // namespace pflib::algorithm
