#pragma once
#ifndef PFTOOL_TRIG_FWHISTO_H
#define PFTOOL_TRIG_FWHISTO_H

#include <array>
#include <nlohmann/json.hpp>
#include <optional>

#include "pflib/zcu/UIO.h"
#include "pflib/logging/Logging.h"

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

  /// type of clock we'll use to measure collection time
  using the_clock = std::chrono::high_resolution_clock;
  /// a time point for our clock
  using a_time_point = std::chrono::time_point<the_clock>;
  /// last time the clear method was called
  std::optional<a_time_point> time_of_last_clear_;

  /// share logging channel with histo menu
  mutable pflib::logging::logger the_log_;

  /// get the collection time
  std::optional<double> get_collection_time(a_time_point now) const;

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
   * the blocks of histograms are logically distinct by
   * what values we fill into them
   */
  enum class FillValue : int {
    /// the sums as output by the decoder lut
    DecodedSum = 0,
    /// the sums as unpacked but still in their 5E+4M encoding
    EncodedSum = 1,
    /// the test histogram codes
    Test = 2
  };

  /**
   * read a histogram's content from the firmware
   *
   * @param[in] fill_type block of histograms to read from
   * @param[in] index index of the histogram we want in that block
   * @return histogram content
   */
  std::array<uint32_t, 256> read_fw(FillValue fill_type, int index);

  /**
   * A single histogram read into memory
   */
  class SingleChannelHistogram {
    /// what type of value was filled into the histogram
    FillValue fill_type_;
    /// index of STC in this histogram
    int index_;
    /// content of the histogram
    std::array<uint32_t, 256> values_;
    /// time histogram was filling in seconds (or zero if no last clear)
    double collection_time_;
   public:
    SingleChannelHistogram(FillValue fill_type, int index, std::array<uint32_t, 256> values, double collection_time);
    /// which type of fill value was put into the histogram
    FillValue fill_type() const;
    /// which index was read
    int index() const;
    /// access the values of the histogram
    const std::array<uint32_t, 256> values() const;
    /// how long was the histogram being filled?
    double collection_time() const;
    /**
     * convert the histogram into
     * [UHI JSON](https://uhi.readthedocs.io/en/latest/serialization.html#json)
     *
     * This JSON output is helpful for loading into python plotting.
     * If only the output from this function is written to the file,
     * then the histogram can be loaded with
     * ```python
     * import json
     * import uhi.io.json
     * import hist
     *
     * with open('path/to/hist.json') as f:
     *     h_ir = json.load(f, object_hook=uhi.io.json.object_hook)
     *
     * h = hist.Hist(h_ir)
     * # h is a 1D histogram of the STC sum
     * ```
     *
     * @return JSON representation of histogram
     */
    nlohmann::json to_json() const;
  };

  /**
   * read the current values for the input histogram index
   * of the input value type
   *
   * @param[in] fill_type FillValue choosing which block of histograms
   * to read from
   * @param[in] ihist index of histogram within that block [0,7]
   * @return the current histogram
   */
  SingleChannelHistogram read(FillValue fill_type, int ihist);

  /**
   * a full histogram block read into memory
   */
  class BlockHistogram {
    /// what type of value was filled into the histogram
    FillValue fill_type_;
    /// content of the histogram
    std::array<std::array<uint32_t, 256>, 8> values_;
    /// time histogram was filling in seconds (or zero if no last clear)
    double collection_time_;
   public:
    BlockHistogram(FillValue fill_type, std::array<std::array<uint32_t, 256>, 8> values, double collection_time);
    /// which type of fill value was put into the histogram
    FillValue fill_type() const;
    /// access the values of the histogram
    const std::array<std::array<uint32_t, 256>, 8> values() const;
    /// how long was the histogram being filled?
    double collection_time() const;

    /**
     * convert the histogram into UHI JSON
     *
     * This JSON output is helpful for loading into python plotting.
     * If only the output from this function is written to the file,
     * then the histogram can be loaded with
     * ```python
     * import json
     * import uhi.io.json
     * import hist
     *
     * with open('path/to/hists.json') as f:
     *     h_ir = json.load(f, object_hook=uhi.io.json.object_hook)
     *
     * h = hist.Hist(h_ir)
     * # h is a 2D histogram where the first axis is the STC
     * # and the second axis is the sum
     * ```
     *
     * The collection time is included in the histogram's 'metadata'
     * in the JSON for scaling the plot later if desired.
     *
     * @return JSON representation of list of histograms
     */
    nlohmann::json to_json() const;
  };

  /**
   * read the current values of a block of histograms of
   * the input value type
   *
   * @param[in] fill_type FillValue choosing which block of histograms to read
   * @return the block histogram read in
   */
  BlockHistogram read(FillValue fill_type);
};

#endif
