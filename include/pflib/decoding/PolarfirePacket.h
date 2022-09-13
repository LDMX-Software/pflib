#ifndef pflib_decoding_PolarfirePacket_h
#define pflib_decoding_PolarfirePacket_h 1

#include "pflib/decoding/LinkPacket.h"

namespace pflib {
namespace decoding {

/** 
 * wrap the encoded binary data readout by a single polarfire
 *
 * This is representing Table 3 from the \dataformats manual.
 * **Remember** Table 3 references "ROC Subpackets" which are really
 * LinkPacket.
 */
class PolarfirePacket {
 public:
  /**
   * Wrap the input array and its lenth as a PolarfirePacket
   * @param[in] header_ptr pointer to beginning of data array
   * @param[in] len length of data array
   */
  PolarfirePacket(const uint32_t* header_ptr, int len);

  /**
   * Get the length of this packet as reported in the first header word
   * @return length, -1 if packet is empty
   */
  int length() const;

  /**
   * Get the number of links readout in this packet as reported in header
   * @return number of links, -1 if packet is empty
   */
  int nlinks() const;

  /**
   * Get the ID of this polarfire
   * @return id number, -1 if packet is empty
   */
  int fpgaid() const;

  /**
   * Get the format version of this polarfire daq format
   * @return version, -1 if packet is empty
   */
  int formatversion() const;

  /**
   * Get the length for a input elink
   * @param[in] ilink link index within this polarfire
   * @return length of that link, 0 if link doe snot exist
   */
  int length_for_elink(int ilink) const;
    
  /**
   * Get BX ID of this readout packet
   * @return bxid, -1 if packet is empty
   */
  int bxid() const;

  /**
   * Get RREQ of this readout packet
   * @return read request, -1 if packet is empty
   */
  int rreq() const;

  /**
   * Get the orbit number from this readout packet
   * @return orbit, -1 if packet is empty
   */
  int orbit() const;

  /**
   * Get the link packet wrapped with our decoding class
   * @param[in] ilink link index within this polarfire
   * @return LinkPacket decoding that link, empty packet if link does not exist
   */
  LinkPacket link(int ilink) const;
 private:
  /**
   * Calculate the offset from the start of the polarfire packet
   * to the input link packet
   * @param[in] ilink link index within this polarfire
   * @return offset, -1 if link does not exist
   */
  int offset_to_elink(int ilink) const;
  /// header pointer to data array
  const uint32_t* data_;
  /// length of data array
  int length_;
};
  
}
}

#endif// pflib_decoding_PolarfirePacket_h
  
