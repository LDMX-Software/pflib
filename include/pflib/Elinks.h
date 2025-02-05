#ifndef pflib_Elinks_h_
#define pflib_Elinks_h_

#include <vector>
#include <stdint.h>

namespace pflib {

/**
 * Interface for configuring the elinks
 */
class Elinks {
  
 protected:
  /**
   */
  Elinks(int nlinks);

public:
  /**
   * Get the number of links stored in this class
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
  virtual std::vector<uint32_t> spy(int ilink) = 0;

  /**
   * set the bitslip value for the link
   *
   * @param[in] ilink link index
   * @param[in] bitslip value for bitslip
   */
  virtual void setBitslip(int ilink, int bitslip) = 0;

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
  virtual void setBitslipAuto(int ilink,bool enable) { }

  /**
   * check if a link is auto-setting the bitslip
   *
   * @param[in] ilink link index
   * @return true if auto-setting is enabled
   */
  virtual bool isBitslipAuto(int ilink) { return false; }

  /**
   * Get the bitslip value for a given link
   *
   * @param[in] ilink link index
   * @return value of bitslip
   */
  virtual int getBitslip(int ilink) = 0;

  /**
   * Get the status of the input link
   *
   * @param[in] ilink link index
   * @return encoded 4-bytes of link status
   */
  virtual uint32_t getStatusRaw(int ilink) = 0;

  /**
   * Clear the error counters for the input link
   *
   * @param[in] ilink link index
   */
  virtual void clearErrorCounters(int ilink) = 0;

  /**
   * Decode the counters for non-idles and resyncs from the status
   * bytes for the input link
   *
   * @param[in] link link index
   * @param[out] nonidles set to the number of non-idles
   * @param[out] resyncs set to the number or resyncs
   */
  virtual void readCounters(int link, uint32_t& nonidles, uint32_t& resyncs) { }

  /**
   * Hard reset the links
   */
  virtual void resetHard() = 0;

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
  //  void setupBigspy(int mode, int ilink, int presamples);

  /**
   * Read the current setup for a bigspy
   *
   * @param[out] mode type of bigspy to setup
   * @param[out] ilink link index
   * @param[out] presamples number of presamples
   */
  //  void getBigspySetup(int& mode, int& ilink, int& presamples);

  /**
   * Check if the current bigspy is done
   * @return true if we are done with the bigspy
   */
  //  bool bigspyDone();  

  /**
   * Readout the bigspy and return the data
   * @return vector of 4-byte words readout from bigspy
   */
  //  std::vector<uint32_t> bigspy();
  
  /**
   * scan the input link attempting to align it
   * @param[in] ilink link index
   */
  //  void scanAlign(int ilink);

  /**
   * Set the l1a delay for the input link
   * @param[in] ilink link index
   * @param[in] idelay delay to use
   */
  virtual void setDelay(int ilink, int idelay) { }
private:
  /// number of links available, read from chip
  int n_links;
  /// which links are "active", set by user
  std::vector<bool> m_linksActive;
};

  // factories
  Elinks* create_Elinks_zcu(bool daq);
  
}

#endif // pflib_Elinks_h_
