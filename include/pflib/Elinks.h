#ifndef pflib_Elinks_h_
#define pflib_Elinks_h_

#include <stdint.h>

#include <vector>

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
    if (ilink < n_links && ilink >= 0) m_linksActive[ilink] = active;
  }

  /**
   * Check if a link is active
   *
   * @param[in] ilink link index
   * @return true if link is active, false if invalid index or link is inactive
   */
  bool isActive(int ilink) const {
    return (ilink >= n_links || ilink < 0) ? (false) : (m_linksActive[ilink]);
  }

  /**
   * spy into the passed link
   *
   * @param[in] ilink link index
   * @return the bytes retreived from the spy
   */
  virtual std::vector<uint32_t> spy(int ilink) = 0;

  /**
   * set the bitslip value (word-level adjustment)
   *
   * @param[in] ilink link index
   * @param[in] bitslip value for bitslip
   */
  virtual void setBitslip(int ilink, int bitslip) = 0;

  /**
   * enable auto-setting of bitslip value (word-level adjustment)
   *
   *
   * @param[in] ilink link index
   */
  virtual int scanBitslip(int ilink);

  /*
   * Get the bitslip value for a given link (word-level adjustment)
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
  virtual void readCounters(int link, uint32_t& nonidles, uint32_t& resyncs) {}

  /**
   * Hard reset the links
   */
  virtual void resetHard() = 0;

  /**
   * scan the input link attempting to bitalign it
   * @param[in] ilink link index
   */
  virtual int scanAlign(int ilink, bool debug = false);

  /**
   * Set the bitalign delay for the input link 
   * @param[in] ilink link index
   * @param[in] idelay delay to use
   */
  virtual void setAlignPhase(int ilink, int iphase) {}

  /**
   * Get the bit alignment phase
   */
  virtual int getAlignPhase(int ilink) { return -1; }

 private:
  /// number of links available, read from chip
  int n_links;
  /// which links are "active", set by user
  std::vector<bool> m_linksActive;
};

// factories
Elinks* get_Elinks_zcu();

}  // namespace pflib

#endif  // pflib_Elinks_h_
