#ifndef pflib_decoding_LinkPacket_h
#define pflib_decoding_LinkPacket_h 1

#include <stdint.h>

namespace pflib {
namespace decoding {

/**
 * smallest formatted packet being readout by the HGCROC-Polarfire pipeline
 *
 * @note Confusingly, these packets correspond to 'ROC Subpackets' in the \dataformats manual.
 * They are detailed in Table 4 of that document.
 *
 * Essentially we do live interpretations of an array of encoded 32-bit ints that
 * are being held somewhere else. For this reason, we are a light wrapper class and
 * so we can we constructed/desctructed freely.
 */
class LinkPacket {
 public:
  /**
   * Wrap the input C-style array as a LinkPacket
   */
  LinkPacket(const uint32_t* header_ptr, int len);

  /**
   * Get the link ID for this link
   *
   * @return link id, -1 if link packet is empty
   */
  int linkid() const;

  /**
   * Get the CRC checksum for this link
   *
   * @return crc, -1 if link packet is empty
   */
  int crc() const;

  /**
   * Get the BX ID as reported by the HGC ROC reading out this link
   *
   * @return bx ID, -1 if link packet is empty
   */
  int bxid() const;

  /**
   * Get the WADD as reported by the HGC ROC reading out this link
   *
   * @return wadd, -1 if link packet is empty
   */
  int wadd() const;

  /**
   * Get the length of this link packet
   * @return length of link packet defined at construction
   */
  int length() const; 

  /**
   * Check if this link has a good BX header
   *
   * If the link packet is empty, it does **not** have good BX header.
   *
   * @return true if BX header matches expected pattern if link is well aligned
   */
  bool good_bxheader() const; 

  /**
   * Check if this link has a good trailing idle
   *
   * If the link packet is not of the correct, full length, it does **not** have a good idle.
   *
   * @return true if idle matches expected pattern for if link is well aligned
   */
  bool good_idle() const; 

  /**
   * Check if a channel exists by attempting to comput its offset
   *
   * @note Uses **in-link** channel numbering i.e. from 0 to 35.
   * @see offset_to_chan for how the channel offset is calculated
   *
   * @return true if a channel is in this link packet
   */
  bool has_chan(int ichan) const;
 
  /**
   * Get the decoded TOT value for the input channel
   * @param[in] ichan in-link channel index
   * @return TOT value in that channel
   */
  int get_tot(int ichan) const; 

  /**
   * Get the decoded TOA value for the input channel
   * @param[in] ichan in-link channel index
   * @return TOA value in that channel
   */
  int get_toa(int ichan) const;
  
  /**
   * Get the decoded ADC value for the input channel
   * @param[in] ichan in-link channel index
   * @return ADC value in that channel
   */
  int get_adc(int ichan) const; 

  /**
   * Print human-readable/decoded link packet to terminal
   */
  void dump() const;
  
 private:
  /**
   * Calculate the offset to a specific channel index by referencing
   * the readout map for this link packet
   *
   * @param[in] ichan in-link channel index
   * @return offset, -1 if channel was not readout or is invalid
   */
  int offset_to_chan(int ichan) const;
  /// handle to zero'th entry in data array we are wrapping
  const uint32_t* data_;
  /// length of data array we have wrapped
  int length_;
}; // LinkPacket

} // namespace decoding
} // namespace pflib

#endif// pflib_decoding_LinkPacket_h
  
