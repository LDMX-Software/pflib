#pragma once
#ifndef PFTOOL_TRIG_FWHISTO_H
#define PFTOOL_TRIG_FWHISTO_H

#include <array>
#include <nlohmann/json.hpp>
#include <optional>

#include "pflib/zcu/UIO.h"

/**
 * read and print histograms filled by the firmware
 */
class FWHistoPool {
  /// register where we write which histogram address we want to read
  static const uint32_t ADDR_REG = 0x604 / 4;
  /// mask for writing histogram addresses
  static const uint32_t ADDR_MASK = 0x3FF0000;
  /// firmware version being read from (updated in constructor)
  static uint32_t FW_VERSION;
  /// handle to register area of trigpath firmware block
  pflib::UIO uio_;

 public:
  /// construct the pool for the passed trigpath index (0 or 1)
  FWHistoPool(int i_trigpath);
  /// reset all of the histograms in the pool to zero counts
  void clear();
  /**
   * use the test histograms to check the functionality of the histogram block
   *
   * The fill value is split across the four test histograms.
   * The lowest 2 bits go into test histogram 0.
   * The next lowest 2 bits go into test histogram 1 multiplied by 4.
   * The next lowest 2 bits go into test histogram 2 multiplied by 16.
   * The next lowest 2 bits go into test histogram 3 multiplied by 64.
   *
   * For example, a value of 85 (= 0b01010101) should cause
   * bin 1 of histogram 0, bin 4 of histogram 1, bin 16 of histogram 2,
   * and bin 64 of histogram 2 to increment by one.
   *
   * After doing this single fill, all of the test histograms are
   * printed to the terminal to view the resulting values.
   *
   * @note Since the fill happens on each BX clock and the register
   * that is normally being used to fill the test histograms is zero,
   * the zero-bin should also have a large number in it.
   */
  void debug(int fill_val);

  /**
   * read the current values for the input histogram index
   *
   * The first eight indices [0,7] map onto the associated STCs.
   * The next four [8,11] are the four test histograms.
   *
   * @return the current histogram values as an array
   */
  std::array<uint32_t, 256> read(int ihist);

  /**
   * convert the input array and histogram index into
   * [UHI JSON](https://uhi.readthedocs.io/en/latest/serialization.html#json)
   *
   * This JSON output is helpful for loading into python plotting.
   * ```python
   * import json
   * import uhi.io.json
   * import hist
   *
   * with open('path/to/hist.json') as f:
   *     h_ir = json.load(f, object_hook=uhi.io.json.object_hook)
   *
   * h = hist.Hist(h_ir)
   * ```
   *
   * @param[in] hist histogram to serialize into UHI JSON
   * @param[in] ihist histogram index to include in labeling
   * @return JSON representation of histogram
   */
  static nlohmann::json to_json(const std::array<uint32_t, 256>& hist,
                                int ihist);

  /**
   * convert the input set of many histograms into a JSON
   * list of UHI JSON histograms
   *
   * This JSON output is helpful for loading into python plotting.
   * ```python
   * import json
   * import uhi.io.json
   * import hist
   *
   * with open('path/to/hists.json') as f:
   *     hist_list = json.load(f, object_hook=uhi.io.json.object_hook)
   *
   * hists = [hist.Hist(h) for h in hist_list]
   * ```
   *
   * @param[in] data set of histograms to serialize into JSON
   * @return JSON representation of list of histograms
   */
  static nlohmann::json to_json(
      const std::array<std::array<uint32_t, 256>, 8>& data);
};

#endif
