#ifndef pflib_decoding_SuperPacket_h
#define pflib_decoding_SuperPacket_h 1

#include "pflib/decoding/PolarfirePacket.h"

namespace pflib {
namespace decoding {

/**
 * Decoding of entire "super packet" which holds multiple polarfire packets - 
 * one for each sample in an event.
 *
 * This class handles replicating the behavior described in Appendix A and B
 * in the \dataformats manual.
 *
 * @note several header words (including the timestamp, local event number,
 * local spill number, and run timestamp) were added and are **not** currently
 * document in the manual linked above.
 */
class SuperPacket {
 public:
  /**
   * Wrap the input data array and search for the header
   *
   * The header word allows us to deduce the format version.
   * Decrement the length and increment the header pointer
   * until the header words is found so that it aligns with
   * the index i in the Appendix tables.
   *
   * @param[in] header_ptr start of data array
   * @param[in] len length of data array
   */
  SuperPacket(const uint32_t* header_ptr, int len);
  
  /**
   * Length of this packet in 64-bit words
   * @see length32 for decoding
   * @return length, 0 if empty packet
   */
  int length64() const;

  /**
   * Length of this packet in 32-bit words
   * @return length, -1 if empty packet
   */
  int length32() const;

  /**
   * Get polarfire fpga id for this super packet
   * @return id number, -1 if empty packet
   */
  int fpgaid() const;

  /**
   * Get the number of samples within this packet
   * @return number of samples, -1 if empty packet
   */
  int nsamples() const;

  /**
   * Get the format version stored within this packet
   * @return version, -1 if empty packet
   */
  int formatversion() const;

  /**
   * Get the length of the event tag
   * @note only availabe in v2 so we check
   * @return tag lenght, 0 if unavailable
   */
  int event_tag_length() const;

  /**
   * Get the spill this packet was for
   * @note local to the polarfire
   * @return spill number, 0 if unavailable
   */
  int spill() const;

  /**
   * Get the BX id this packet was for
   * @note local to the polarfire
   * @return bxid, 0 if unavailable
   */
  int bxid() const;

  /**
   * Get the time in 5MHz counts since spill begun
   * @return time, 0 if unavailable
   */
  uint32_t time_in_spill() const;

  /**
   * Get local event ID for this packet
   * @return event id, 0 if unavailable
   */
  uint32_t eventid() const;

  /**
   * Get run ID for this packet
   * @return run, 0 if unavailable
   */
  int runid() const;

  /**
   * Set the run start timestamp for this packet
   * @param[out] month run start month, unmodified if unavailable
   * @param[out] day run start day, unmodified if unavailable
   * @param[out] hour run start hour, unmodified if unavailable
   * @param[out] minute run start minute, unmodified if unavailable
   */
  void run_start(int& month, int& day, int& hour, int& minute);
  
  /**
   * Get length of an individual sample in this packet
   * @param[in] isample sample index within this packet
   * @return length of that sample in 32-bit words, 0 if sample does not exist
   */
  int length32_for_sample(int isample) const;
    
  /**
   * Get a specific sample from this packet, wrapping with our decoding class
   * @param[in] isample sample index within this packet
   * @return decoding polarfire packet for the input sample index, empty if not exists
   */
  PolarfirePacket sample(int isample) const;

  /**
   * Get the offset between the input pointer to the constructor
   * and the start of the packet
   * @return offset
   */
  int offset_to_header() const { return offset_; }
 private:
  /// pointer to beginning of packet
  const uint32_t* data_;
  /// length of packet
  int length_;
  /// format version of packet, deduced from header word
  int version_;
  /// offset between input data array pointer and beginning of packet
  int offset_;
};
  
}
}

#endif// pflib_decoding_PolarfirePacket_h
  
