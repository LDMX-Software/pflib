#pragma once
#ifndef PFLIB_PACKING_DAQSAMPLEHEADER
#define PFLIB_PACKING_DAQSAMPLEHEADER

#include <cstdint>
#include <ostream>

namespace pflib::packing {

/**
 * The header that is inserted by the DAQ firmware (on the Bittware)
 * or emulated by software (on the ZCU)
 *
 * This needs to be its own class since it is used in both
 * MultiSamleECONDEventPacket::from and MultiSampleECONDEventPacket::read
 * as well as DAQ::read_event_sw_header
 *
 * 4b vers | 10b ECON ID | 5b il1a | S | 12b length
 *
 * - vers is the format version
 * - ECOND ID is what it was configured in the software to be
 * - il1a is the index of the sample relative to this event
 * - S signals if this is the sample of interest (1) or not (0)
 * - length is the total length of this link subpacket NOT including this
 *   header
 */
struct DAQSampleHeader {
  uint32_t version;
  uint32_t econd_id;
  uint32_t i_l1a;
  bool is_soi;
  uint32_t econd_len;

  /// decode a DAQSampleHeader from the input word
  void from(uint32_t word);

  /// construct a header word from our data
  uint32_t to() const;

  /**
   * output stream operator to make logging easier
   */
  friend std::ostream& operator<<(std::ostream& o, const DAQSampleHeader& h);

  /**
   * A special form of this DAQ header is used to signal
   * the end of a multi-sample sequence.
   *
   * Both i_l1a and econd_id are set to their maximum values.
   */
  bool is_ending_trailer() const;

  /**
   * Construct the special trailer form of this header
   */
  static uint32_t ending_trailer();
};

}

#endif
