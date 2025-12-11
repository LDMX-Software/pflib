#include "pflib/packing/MultiSampleECONDEventPacket.h"

#include "pflib/packing/Hex.h"
#include "pflib/packing/Mask.h"

namespace pflib::packing {

MultiSampleECONDEventPacket::MultiSampleECONDEventPacket(int n_links)
    : n_links_{n_links} {}

const std::string MultiSampleECONDEventPacket::to_csv_header =
    "timestamp,orbit,bx,event,i_link,channel,i_sample,Tp,Tc,adc_tm1,adc,tot,toa";

void MultiSampleECONDEventPacket::to_csv(std::ofstream& f) const {
  /**
   * The columns of the output CSV are
   * ```
   * timestamp,orbit,bx,event,i_link,channel,i_sample,Tp,Tc,adc_tm1,adc,tot,toa
   * ```
   */
  for (std::size_t i_sample{0}; i_sample < samples.size(); i_sample++) {
    const auto& sample{samples[i_sample]};
    for (std::size_t i_link{0}; i_link < sample.links.size(); i_link++) {
      const auto& daq_link{sample.links[i_link]};
      f << timestamp << ',' 
        << daq_link.orbit << ','
        << daq_link.bx << ','
        << daq_link.event << ','
        << i_link << ','
        << "calib,"
        << i_sample << ',';
      daq_link.calib.to_csv(f);
      f << '\n';
      for (std::size_t i_ch{0}; i_ch < 36; i_ch++) {
        f << timestamp << ',' 
          << daq_link.orbit << ','
          << daq_link.bx << ','
          << daq_link.event << ','
          << i_link << ','
          << i_ch << ','
          << i_sample << ',';
        daq_link.channels[i_ch].to_csv(f);
        f << '\n';
      }
    }
  }
}

void MultiSampleECONDEventPacket::from(std::span<uint32_t> frame, bool expect_ldmx_ror_header) {
  samples.clear();
  contrib_id = 0;
  subsys_id = 0;
  timestamp = 0;

  std::size_t offset{0};
  if (expect_ldmx_ror_header) {
    /**
     * the ldmx ror header inserts 4 32-bit words (16 bytes) of the form
     *
     * 0xA5 | 8b contrib | 8b subsys | 8b VERS = 0
     * 32b 0
     * 64b timestamp
     */

    uint8_t sentinel = static_cast<uint8_t>((frame[0] >> 24) & 0xff);
    contrib_id = ((frame[0] >> 16) & 0xff);
    subsys_id = ((frame[0] >> 8) & 0xff);
    uint8_t vers = static_cast<uint8_t>(frame[0] & 0xff);

    //frame[1] == 0

    timestamp = ((frame[3] << 32) | frame[2]);
    offset += 4;
  }
  std::size_t i_sample{0};
  while (offset < frame.size()) {
    /**
     * The software emulation adds another header before the ECOND packet,
     * which looks like
     *
     * 4b vers | 10b ECON ID | 5b il1a | S | 0 | 8b length
     *
     * - vers is the format version
     * - ECOND ID is what it was configured in the software to be
     * - il1a is the index of the sample relative to this event
     * - S signals if this is the sample of interest (1) or not (0)
     * - length is the total length of this link subpacket NOT including this
     */
    uint32_t vers = ((frame[offset] >> 28) & mask<4>);
    uint32_t new_econd_id = ((frame[offset] >> 18) & mask<10>);
    uint32_t il1a = ((frame[offset] >> 13) & mask<5>);
    bool is_soi = (((frame[offset] >> 12) & mask<1>) == 1);
    uint32_t econd_len = (frame[offset] & mask<8>);

    if (not is_soi and il1a == 31 and new_econd_id == 1023) {
      pflib_log(trace) << "Last sample packet found, leaving loop at offset = "
                       << offset << " (frame.size() = " << frame.size() << ")";
      break;
    }

    if (i_sample > 0 and econd_id != new_econd_id) {
      pflib_log(warn) << "ECON ID mismatch: Found " << new_econd_id
                      << " but this stream was " << econd_id << " earlier";
    }
    econd_id = new_econd_id;
    pflib_log(trace) << hex(frame[offset])
                     << " -> econd_len, il1a, econd_id, is_soi = " << econd_len
                     << ", " << il1a << ", " << econd_id << ", " << is_soi;

    // header decoded, shift offset
    offset++;

    if (i_sample != il1a) {
      pflib_log(warn) << "mismatch between transmitted index and unpacking "
                         "index for sample "
                      << i_sample << " != " << il1a;
    }

    if (is_soi) {
      i_soi = i_sample;
    }

    samples.emplace_back(n_links_);
    samples.back().from(frame.subspan(offset, econd_len));
    offset += econd_len;

    i_sample++;
  }
}

Reader& MultiSampleECONDEventPacket::read(Reader& r) {
  /**
   * DANGER
   * Without signal header/trailer words, this assumes that the data
   * stream is word aligned and we aren't starting on the wrong word.
   */
  std::vector<uint32_t> frame;
  while (r) {
    uint32_t word{0};
    if (!(r >> word)) {
      // is this worthy of a warning?
      pflib_log(trace)
          << "leaving frame accumulation loop failing to pop next header word";
      break;
    }
    frame.push_back(word);
    uint32_t vers = ((word >> 28) & mask<4>);
    uint32_t new_econd_id = ((word >> 18) & mask<10>);
    uint32_t il1a = ((word >> 13) & mask<5>);
    bool is_soi = (((word >> 12) & mask<1>) == 1);
    uint32_t econd_len = word & mask<8>;
    pflib_log(trace) << hex(word)
                     << " -> vers, econd_len, il1a, econd_id, is_soi = " << vers
                     << ", " << econd_len << ", " << il1a << ", "
                     << new_econd_id << ", " << is_soi;
    if (il1a == 31 and new_econd_id == 1023) {
      pflib_log(trace) << "found special header marking end of packet"
                       << ", leaving accumulation loop";
      break;
    }
    if (!r.read(frame, econd_len, frame.size())) {
      pflib_log(warn) << "partially transmitted frame!";
      return r;
    }
    pflib_log(trace) << hex(*(frame.end() - 1));
  }

  this->from(frame);
  return r;
}

const ECONDEventPacket& MultiSampleECONDEventPacket::soi() const {
  return samples.at(i_soi);
}

}  // namespace pflib::packing
