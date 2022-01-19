#ifndef PFLIB_POLARFIRETARGET_H
#define PFLIB_POLARFIRETARGET_H

#include <ostream>
#include <memory>
#include <functional>

#include "pflib/WishboneInterface.h"
#include "pflib/Backend.h"
#include "pflib/Hcal.h"

namespace pflib {

/**
 * Interface to a single polarfire.
 *
 * This class holds the objects that talk with the polarfire
 * and defines higher-level functionality allowing for automatic
 * initialization, configuration, and running.
 *
 * In order to assist in developing the helpful debugging tool 'pftool',
 * the members of this class will also be public.
 */
struct PolarfireTarget {
  std::shared_ptr<WishboneInterface> wb;
  std::shared_ptr<Backend> backend;
  std::unique_ptr<Hcal> hcal;
  static const int TGT_CTL;
  static const int TGT_ROCBUF;
  static const int TGT_FMT;
  static const int TGT_BUFFER;
  // need to read this eventually
  static int NLINKS;

  /**
   * Define where the polarfire we will be talking to is.
   */
  PolarfireTarget(const std::string& host, int port);

  /**
   * deduce firmware major/minor version
   * @return {major,minor} pair
   */
  std::pair<int,int> getFirmwareVersion();

  /**
   * Entire ldmx_link function commented out pretty much..
   */

  /**
   * Do a "big spy" on the input elinks.
   * We optionally allow for the bigspy to be triggered with an L1A
   * (do_l1a == true) or just spy immediately (do_l1a == false).
   */
  std::vector<uint32_t> elinksBigSpy(int ilink, int presamples, bool do_l1a);

  /**
   * Print a decoded elink status to the input ostream
   *  MOVE TO ELINKS
   */
  void elinkStatus(std::ostream& os);

  /**
   * Load an integer CSV and pass the rows one at a time to the input Action
   *
   * Action is a function that looks like:
   * ```cpp
   * void Action(const std::vector<int>&)
   * ```
   * or a lambda like
   * ```cpp
   * [](const std::vector<int>&) {
   *  //blah
   * }
   * ```
   *
   * @return false if unable to open file
   */
  bool loadIntegerCSV(const std::string& file_name, 
      const std::function<void(const std::vector<int>&)>& Action);

  /**
   * ## Files ending in .csv
   * Defines an Action with calls ROC::setValue if there
   * are three cells and prints a warning otherwise.
   *
   * ## Files ending in .yaml or .yml
   * Prints a not-implemented error.
   *
   * ## Other extensions
   * Prints a error.
   *
   * @return false if unable to open file
   */
  bool loadROCSettings(int roc, const std::string& file_name);

  void prepareNewRun();

  /**
   * Print a decoded daq status to the input ostream
   *  MOVE TO DAQ
   */
  void daqStatus(std::ostream& os);

  /**
   * Enable zero suppression in the input link.
   * link == -1 is all links
   */
  void enableZeroSuppression(int link, bool full_suppress);

  /**
   * daq soft reset
   *  stays here becauses uses FC as well as DAQ
   */
  void daqSoftReset();

  /**
   * daq hard reset
   *  stays here because uses backend
   */
  void daqHardReset();

  /**
   * daq read direct
   *  uses backend
   */
  std::vector<uint32_t> daqReadDirect();

  /**
   * daq read event
   *  make sure to prepareNewRun before!
   */
  std::vector<uint32_t> daqReadEvent();

  /**
   * set a single bias setting
   */
  void setBiasSetting(int board, bool led, int hdmi, int val);

  /**
   * ## Files ending in .csv
   * Defines an Action with calls setBiasSetting
   * if there are four columns.
   * [board,0=SiPM/1=LED,hdmi#,value]
   *
   * Prints a warning on non-four columns
   * returns false if unable to open file
   *
   * ## Files ending in .yaml or .yml
   * Prints a not-implemented error.
   *
   * ## Other extensions
   * Prints a error.
   *
   * @return false if unable to open file
   */
  bool loadBiasSettings(const std::string& file_name);
};

}

#endif
