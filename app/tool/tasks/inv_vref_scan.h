#pragma once

#include "../pftool.h"

/**
 * TASKS.INV_VREF_SCAN
 *
 * Perform INV_VREF scan for each link
 * used to trim adc pedestals between two links
 */
class DataFitter
{
  public:
    DataFitter();

    // Member functions

    void sort_and_append(std::vector<int>& inv_vrefs,
                         std::vector<int>& pedestals,
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
    int RH_median_;

};

void inv_vref_scan(Target* tgt);
