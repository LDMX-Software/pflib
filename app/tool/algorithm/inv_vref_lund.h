#pragma once

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
 * a linear region in the INV_VREF parameter space, and then appends to each Point
 * fit() fits the linear region according to a standard linear fit.
 *
 */
class DataFitter
{
  public:
    DataFitter();

    // Member functions

    void sort_and_append(std::vector<int>& inv_vrefs,
                         std::vector<int>& pedestals,
                         std::vector<double>& stds,
                         int& step);
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
    int LH_median_;
    double LH_std_median_;
    int RH_median_;

};

/**
 * Find the inv_vref parameters. The noinv_vref parameters are 
 * hardcoded to 612 for each link.
 *
 * @param[in] tgt pointer to Target to interact with
 *
 * @note Only functional for single-ROC targets
 */
std::map<std::string, std::map<std::string, uint64_t>> inv_vref_lund(
    Target* tgt, ROC& roc);

}  // namespace pflib::algorithm
