#include "pflib/packing/MultiSampleECONDEventPacket.h"

#include "pflib/packing/Hex.h"
#include "pflib/packing/Mask.h"

namespace pflib::packing {

MultiSampleECONDEventPacket::MultiSampleECONDEventPacket(int n_links)
    : n_links_{n_links} {}

const std::string MultiSampleECONDEventPacket::to_csv_header =
    "i_link,bx,event,orbit,channel," + Sample::to_csv_header;

void MultiSampleECONDEventPacket::from(std::span<uint32_t> data) {
  samples.clear();
  run = data[0];
  pflib_log(trace) << hex(data[0]) << " -> run = " << run;
  ievent = (data[1] >> 8);
  bx = (data[1] & mask<8>);
  pflib_log(trace) << hex(data[1]) << " -> ievent, bx = " << ievent << ", "
                   << bx;
  uint32_t zero = data[2];  // should be total length?
  pflib_log(trace) << hex(data[2]) << " -> length ?= " << data[2];
  uint32_t head_flag = (data[3] >> 24);
  corruption[0] = (head_flag != 0xA6u);
  if (corruption[0]) {
    pflib_log(warn) << "Header flag is not the same as expected value "
                    << hex<uint32_t, 2>(head_flag) << " != 0xA6";
  }
  contrib_id = ((data[3] >> 16) & mask<8>);
  subsys_id = ((data[3] >> 8) & mask<8>);
  corruption[1] = (subsys_id != 0x07);
  if (corruption[1]) {
    pflib_log(warn) << "Subsystem DAQ ID not equal to HCal ID "
                    << hex<int, 2>(subsys_id) << " != 0x07";
  }
  pflib_log(trace) << hex(data[3]) << " -> contrib, subsys = " << contrib_id
                   << ", " << subsys_id;

  std::size_t i_sample{0};
  std::size_t offset{4};
  while (offset < data.size()) {
    /**
     * The software emulation adds another header before the ECOND packet,
     * which looks like
     *
     * 4b flag | 12b ECON ID | L | 3b il1a | I | 11b length
     *
     * - flag is hardcoded to 0b0001 right now in software
     * - ECOND ID is what it was configured in the software to be
     * - L signals if this is the last sample (1) or not (0)
     * - il1a is the index of the sample relative to this event
     * - I signals if this is the sample of interest (1) or not (0)
     * - length is the total length of this link subpacket including this header
     * word
     */
    uint32_t link_len = (data[offset] & mask<11>);
    uint32_t il1a = (data[offset] >> 13) & mask<3>;
    uint32_t econ_id = ((data[offset] >> 16) & mask<12>);
    // uint32_t head_flag = (data[offset] >> 28);
    bool is_soi = (((data[offset] >> 12) & mask<1>) == 1);
    bool is_last = (((data[offset] >> 15) & mask<1>) == 1);
    // other flags about SOI or last sample
    pflib_log(trace) << hex(data[offset])
                     << " -> link_len, il1a, econ_id, is_soi, is_last = "
                     << link_len << ", " << il1a << ", " << econ_id << ", "
                     << is_soi << ", " << is_last;

    if (i_sample != il1a) {
      pflib_log(warn) << "mismatch between transmitted index and unpacking "
                         "index for sample "
                      << i_sample << " != " << il1a;
    }

    if (is_soi) {
      i_soi = i_sample;
    }

    samples.emplace_back(n_links_);
    samples.back().from(data.subspan(offset + 1, link_len - 1));
    offset += link_len;

    if (is_last) {
      pflib_log(trace) << "Last sample packet found, leaving loop at offset = "
                       << offset << " (data.size() = " << data.size() << ")";
      break;
    }

    i_sample++;
  }
}

Reader& MultiSampleECONDEventPacket::read(Reader& r) {
  uint32_t word{0};
  pflib_log(trace) << "header scan...";
  while (word != 0xb33f2025) {
    if (!(r >> word)) break;
    pflib_log(trace) << hex(word);
  }
  if (!r) {
    pflib_log(trace) << "no header found";
    return r;
  }
  pflib_log(trace) << "found header";

  std::vector<uint32_t> data;
  pflib_log(trace) << "scanning for end of packet...";
  while (word != 0x12345678) {
    if (!(r >> word)) break;
    data.push_back(word);
    pflib_log(trace) << hex(word);
  }
  if (!r and word != 0x12345678) {
    pflib_log(trace) << "no trailer found, probably incomplete packet";
  }

  this->from(data);

  return r;
}

}  // namespace pflib::packing
