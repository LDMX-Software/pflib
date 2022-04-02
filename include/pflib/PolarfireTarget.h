#ifndef PFLIB_POLARFIRETARGET_H
#define PFLIB_POLARFIRETARGET_H

#include <ostream>
#include <memory>
#include <functional>

#include "pflib/WishboneInterface.h"
#include "pflib/Backend.h"
#include "pflib/Hcal.h"

/**
 * Polarfire Interaction Library
 *
 * This library is designed to ease the burden of configuring
 * HGC ROCs connected to a Polarfire FPGA as well as define
 * some helpful debugging functionalities for determining
 * the optimal configuration of an HGC ROC.
 */
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
  /// are the WBI and Backend the same object?
  bool wb_be_same;
  /// handle to opened wishbone interface
  WishboneInterface* wb;
  /// handle to opened backend connection
  Backend* backend;
  /// object representing hcal motherboard
  Hcal hcal;
  /// number of availabe pages on HGC ROC
  static const int N_PAGES;
  /// number of registers per page of HGC ROC
  static const int N_REGISTERS_PER_PAGE;
  /**
   * number of links
   *
   * need to read this eventually
   */
  static int NLINKS;

  /**
   * Define where the polarfire we will be talking to is.
   *
   * @note This object takes ownership of the input pointers.
   * If they are the same pointers to the same object (defined by same), 
   * then only one is deleted in the destructor.
   *
   * @param[in] wbi pointer to object to use as wishbone interface
   * @param[in] be pointer to object to use as backend
   * @param[in] same true if the object pointed to be wbi and be is a single object
   */
  PolarfireTarget(WishboneInterface* wbi, Backend* be, bool same = false);

  /**
   * If only a single pointer is provided, assume that object is BOTH
   * the WBI and backend.
   *
   * This is templated so that this class does not need to change upon
   * the addition of a new wishbone interface/backend combo object.
   *
   * @tparam T class that inherits from both WishboneInterface and Backend
   * @param[in] t pointer to object that is both a wishbone interface and a backend
   */
  template<class T>
  PolarfireTarget(T* t) : PolarfireTarget(t,t,true) {};

  /**
   * Cleanup the wishbone interface and backend
   * checking if they are the same.
   */
  ~PolarfireTarget();

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
  void loadIntegerCSV(const std::string& file_name, 
      const std::function<void(const std::vector<int>&)>& Action);

  /**
   * Load register values onto ROC chip from inptu file
   *
   * Defines an Action with calls ROC::setValue if there
   * are three cells and prints a warning otherwise.
   *
   * @return false if unable to open file
   */
  void loadROCRegisters(int roc, const std::string& file_name);

  /**
   * Compile the input YAML file including the defaults
   * if prepend_defaults is true, otherwise using current
   * ROC settings  as "base" settings, then writes register
   * values onto chip.
   *
   * @return false if unable to open file
   */
  void loadROCParameters(int roc, const std::string& file_name, bool prepend_defaults);

  /**
   * Request all of the ROC setting register values
   *
   * The input lets you choose if you want the output
   * to be "decompiled", i.e. the the registr values are
   * unpacked into the parameter values.
   */
  void dumpSettings(int roc, const std::string& file_name, bool decompile);

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

  /** Carries out the standard elink alignment process */
  void elink_relink(int verbosity);

  void bitslip();
  
 private:
  int samples_per_event_;  
};

}

#endif
