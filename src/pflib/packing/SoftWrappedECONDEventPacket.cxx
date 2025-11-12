#include "pflib/packing/SoftWrappedECONDEventPacket.h"

#include "pflib/packing/Hex.h"
#include "pflib/packing/Mask.h"

namespace pflib::packing {

SoftWrappedECONDEventPacket::SoftWrappedECONDEventPacket(std::size_t n_links)
    : data{n_links} {}

void SoftWrappedECONDEventPacket::from(std::span<uint32_t> frame) {
  /**
   * The software emulation adds another header before the ECOND packet,
   * which looks like
   *
   * 4b flag | 9b ECON ID | 4b il1a | S | 0 | 8b length
   *
   * - flag is hardcoded to 0b0001 right now in software
   * - ECOND ID is what it was configured in the software to be
   * - il1a is the index of the sample relative to this event
   * - S signals if this is the sample of interest (1) or not (0)
   * - length is the total length of this link subpacket including this header
   * word
   */
  std::size_t offset{0};
  uint32_t link_len = (frame[offset] & mask<8>);
  is_soi = (((frame[offset] >> (8 + 4)) & mask<1>) == 1);
  il1a = ((frame[offset] >> (8 + 4 + 1)) & mask<4>);
  contrib_id = ((frame[offset] >> (8 + 4 + 1 + 4)) & mask<9>);
  uint32_t vers = ((frame[offset] >> 28) & mask<4>);
  pflib_log(trace) << hex(frame[offset])
                   << " -> link_len, il1a, contrib_id, is_soi = " << link_len
                   << ", " << il1a << ", " << contrib_id << ", " << is_soi;

  corruption[0] = (vers != 1);
  if (corruption[0]) {
    pflib_log(warn) << "version transmitted in header " << vers << " != 1";
  }

  data.from(frame.subspan(offset + 1, link_len));
}

Reader& SoftWrappedECONDEventPacket::read(Reader& r) {
  /**
   * DANGER
   * Without signal trailer words, this assumes that the data
   * stream is word aligned and we aren't starting on the wrong
   * word.
   */
  uint32_t word{0};
  if (!(r >> word)) {
    return r;
  }
  std::vector<uint32_t> frame = {word};
  uint32_t econd_len = word & mask<8>;
  pflib_log(trace) << "using " << hex(word)
                   << " to get econd length = " << econd_len;
  if (r.read(frame, econd_len, 1)) {
    from(frame);
  }
  return r;
}

}  // namespace pflib::packing
