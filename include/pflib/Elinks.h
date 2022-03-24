#ifndef pflib_Elinks_h_
#define pflib_Elinks_h_

#include "pflib/WishboneTarget.h"
#include <vector>

namespace pflib {

/**
 * WishboneTarget for configuring the elinks
 */
class Elinks : public WishboneTarget {
 public:
  /**
   * wrap the wishbone interface and use the tgt_Elinks target
   */
  Elinks(WishboneInterface* wb, int target = tgt_Elinks);

  /**
   * Get the number of links stored in this class
   *
   * This value is read at construction time using the passed WishboneInterface
   */
  int nlinks() const { return n_links; }

  /**
   * Mark a specific link as active (or inactive) depending on input
   *
   * If the passed link index is outside the range of valid links,
   * then this does nothing.
   *
   * @param[in] ilink link index
   * @param[in] active true if link is active
   */
  void markActive(int ilink, bool active) { 
    if (ilink<n_links && ilink>=0) 
      m_linksActive[ilink]=active; 
  }
  
  /**
   * Check if a link is active
   *
   * @param[in] ilink link index
   * @return true if link is active, false if invalid index or link is inactive
   */
  bool isActive(int ilink) const { 
    return (ilink>=n_links || ilink<0)?(false):(m_linksActive[ilink]); 
  }
  
  /**
   * spy into the passed link
   *
   * @param[in] ilink link index
   * @return the bytes retreived from the spy
   */
  std::vector<uint8_t> spy(int ilink);

  /**
   * set the bitslip value for the link
   *
   * @param[in] ilink link index
   * @param[in] bitslip value for bitslip
   */
  void setBitslip(int ilink, int bitslip);

  /**
   * enable auto-setting of bitslip value
   *
   * @note Auto-setting the bitslip value for a link is
   * a firmware-level task which is not well implemented at this time.
   *
   * This overrides any manual setting of the bitslip value.
   *
   * @param[in] ilink link index
   * @param[in] enable true if you want the auto-setting enabled
   */
  void setBitslipAuto(int ilink,bool enable);

  /**
   * check if a link is auto-setting the bitslip
   *
   * @param[in] ilink link index
   * @return true if auto-setting is enabled
   */
  bool isBitslipAuto(int ilink);

  /**
   * Get the bitslip value for a given link
   *
   * @param[in] ilink link index
   * @return value of bitslip
   */
  int getBitslip(int ilink);

  /**
   * Get the status of the input link
   *
   * @param[in] ilink link index
   * @return encoded 4-bytes of link status
   */
  uint32_t getStatusRaw(int ilink);

  /**
   * Clear the error counters for the input link
   *
   * @param[in] ilink link index
   */
  void clearErrorCounters(int ilink);

  /**
   * Decode the counters for non-idles and resyncs from the status
   * bytes for the input link
   *
   * @param[in] link link index
   * @param[out] nonidles set to the number of non-idles
   * @param[out] resyncs set to the number or resyncs
   */
  void readCounters(int link, uint32_t& nonidles, uint32_t& resyncs);

  /**
   * Hard reset the links
   */
  void resetHard();

  /** 
   * Prepare for a big spy of the link
   *
   * ### Modes 
   * - 0 -> software
   * - 1 -> L1A
   * - 2 -> Non-idle 
   *
   * @param[in] mode type of bigspy to setup
   * @param[in] ilink link index
   * @param[in] presamples number of presamples
   */
  void setupBigspy(int mode, int ilink, int presamples);

  /**
   * Read the current setup for a bigspy
   *
   * @param[out] mode type of bigspy to setup
   * @param[out] ilink link index
   * @param[out] presamples number of presamples
   */
  void getBigspySetup(int& mode, int& ilink, int& presamples);

  /**
   * Check if the current bigspy is done
   * @return true if we are done with the bigspy
   */
  bool bigspyDone();  

  /**
   * Readout the bigspy and return the data
   * @return vector of 4-byte words readout from bigspy
   */
  std::vector<uint32_t> bigspy();
  
  /**
   * scan the input link attempting to align it
   * @param[in] ilink link index
   */
  void scanAlign(int ilink);

  /**
   * Set the delay for the input link
   * @param[in] ilink link index
   * @param[in] idelay delay to use
   */
  void setDelay(int ilink, int idelay);
 private:
  /// number of links available, read from chip
  int n_links;
  /// which links are "active", set by user
  std::vector<bool> m_linksActive;
  /// UNUSED
  std::vector<int> phaseDelay;
};

}

#endif // pflib_Elinks_h_
