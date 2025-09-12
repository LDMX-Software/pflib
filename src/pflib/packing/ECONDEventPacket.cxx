#include "pflib/packing/ECONDEventPacket.h"

#include "pflib/packing/Hex.h"
#include "pflib/packing/Mask.h"
#include "pflib/utility/crc.h"

#include <boost/crc.hpp>
#include <boost/endian/conversion.hpp>

#include <iostream>

namespace pflib::packing {

std::size_t ECONDEventPacket::unpack_link_subpacket(std::span<uint32_t> data, DAQLinkFrame &link, bool passthrough) {
  pflib_log(trace) << "link header " << hex(data[0]);
  // sub-packet start
  uint32_t stat = ((data[0] >> 29) & mask<3>);
  link.corruption[0] = (stat != 0b111); // all ones is GOOD
  if (link.corruption[0]) {
    pflib_log(warn) << "bad subpacket checks " << std::bitset<3>(stat);
  }
  uint32_t ham = ((data[0] >> 26) & mask<3>);
  bool is_empty = ((data[0] >> 25) & mask<1>) == 1;
  link.adc_cm0 = ((data[0] >> 15) & mask<10>);
  link.adc_cm1 = ((data[0] >>  5) & mask<10>);
  pflib_log(trace) << "    is_empty=" << is_empty
                   << " CM0=" << link.adc_cm0
                   << " CM1=" << link.adc_cm1;

  std::size_t length{0};
  if (is_empty) {
    // F=1
    // single-word header, no channel data or channel map, entire link is empty
    length = 1;
    bool error_caused_empty = ((data[0] >> 4) & mask<1>);
    // E=0 (false) means none of the 37 channels passed zero suppression
    // E=1 (true) means the unmasked Stat error bits caused the sub-packet to be suppressed
    pflib_log(trace) << "is empty, E=" << error_caused_empty;
    return length;
  }

  pflib_log(trace) << "chan map lower 32 " << hex(data[1]);
  // construct channel map
  std::bitset<37> channel_map = data[1];
  channel_map |= ((data[0] & mask<5>) << 32);
  pflib_log(trace) << "is not empty, channel_map=" << channel_map;

  // start length with two header words (including channel map)
  length = 2;

  int bits_left_in_current_word{32};
  for (std::size_t i_chan{0}; i_chan < 37; i_chan++) {
    pflib_log(trace) << "length=" << length << " blic=" << bits_left_in_current_word << " i_ch=" << i_chan;

    // deduce a reference to the channel that we are unpacking
    Sample *ch = &(link.calib);
    if (i_chan < 18) {
      ch = &(link.channels[i_chan]);
    } else if (i_chan > 18) {
      ch = &(link.channels[i_chan-1]);
    }

    if (passthrough) {
      // channels are transparently copied from the link
      pflib_log(trace) << "passthrough " << i_chan << " data " << hex(data[length]);
      ch->word = data[length];
      length++;
      continue;
    }

    // skip channels that are not passed
    if (not channel_map[i_chan]) {
      pflib_log(trace) << "skipping ch " << i_chan << " because its not in map";
      ch->word = 0;
      continue;
    }

    pflib_log(trace) << hex(data[length]);

    // The code cannot span multiple words because the format is byte aligned and we
    // start with a 4 or 2 bit code.
    auto code{(data[length] >> (bits_left_in_current_word - 4)) & mask<4>};
    pflib_log(trace) << "ch " << i_chan << " code = " << std::bitset<4>(code)
      << " (top two = " << std::bitset<2>(code >> 2) << ")";
    // direct transcription of the table in Fig 20 of the ECOND Spec
    int nbits{0}, code_len{0}, extra{0};
    bool has_adctm1{true}, has_toa{true};
    bool Tc{false}, Tp{false};
    if (code == 0b0000) {
      // TOA ZS
      Tc = false;
      Tp = false;
      code_len = 4;
      has_adctm1 = true;
      has_toa = false;
      extra = 0;
      nbits = 24;
    } else if (code == 0b0001) {
      // ADC(-1) and TOA ZS
      Tc = false;
      Tp = false;
      code_len = 4;
      has_adctm1 = false;
      has_toa = false;
      extra = 2;
      nbits = 16;
    } else if (code == 0b0010) {
      // No ZS or ...
      Tc = false;
      Tp = true;
      code_len = 4;
      has_adctm1 = true;
      has_toa = false;
      extra = 0;
      nbits = 24;
    } else if (code == 0b0011) {
      // ADC(-1) ZS
      Tc = false;
      Tp = false;
      code_len = 4;
      has_adctm1 = false;
      has_toa = true;
      extra = 0;
      nbits = 24;
    } else if ((code >> 2) == 0b01) {
      // full pass ZS in ADC mode
      Tc = false;
      Tp = false;
      code_len = 2;
      has_adctm1 = true;
      has_toa = true;
      extra = 0;
      nbits = 32;
    } else if ((code >> 2) == 0b11) {
      // pass ZS because in TOT mode
      Tc = true;
      Tp = true;
      code_len = 2;
      has_adctm1 = true;
      has_toa = true;
      extra = 0;
      nbits = 32;
    } else if ((code >> 2) == 0b10) {
      // known invalid
      pflib_log(debug) << "ECOND eRx known invalid code " << std::bitset<2>(code >> 2);
      Tc = true;
      Tp = false;
      code_len = 2;
      has_adctm1 = true;
      has_toa = true;
      extra = 0;
      nbits = 32;
    } else {
      // unknown code probably meaning the unpacking/decoding is going wrong
      pflib_log(error) << "unrecognized ECOND eRx code " << std::bitset<4>(code);
    }
    pflib_log(trace) << "    nbits=" << nbits;

    // get the channel data from the current and perhaps the next word
    uint32_t chan_data{0};
    if (bits_left_in_current_word < nbits) {
      int runon_length = nbits - bits_left_in_current_word;
      chan_data = ((data[length] & ((1ul << bits_left_in_current_word) - 1ul)) << runon_length);
      length++;
      bits_left_in_current_word = 32;
      chan_data += ((data[length] >> (bits_left_in_current_word - runon_length)) & ((1ul << runon_length) - 1ul));
      bits_left_in_current_word -= runon_length;
    } else {
      chan_data = ((data[length] >> (bits_left_in_current_word - nbits)) & ((1ul << nbits)-1ul));
      bits_left_in_current_word -= nbits;
    }

    pflib_log(trace) << "    chan_data=" << hex(chan_data);

    // unpack the channel data into the measurements that it contains
    // none of these measurements can be negative, so a negative value
    // corresponds to an measurement that did not exist
    int adc_tm1{-1}, toa{-1};

    // shift out padding extra bits
    chan_data >>= extra;

    // next lowest 10 bits are TOA if it has it
    if (has_toa) {
      toa = (chan_data & mask<10>);
      chan_data >>= 10;
    }

    // next lowest 10 bits are the main sample ADC or TOT
    uint32_t main_sample = (chan_data & mask<10>);
    chan_data >>= 10;

    // next lowest 10 bits are the ADCt-1 if it has it
    if (has_adctm1) {
      adc_tm1 = (chan_data & mask<10>);
    }

    pflib_log(trace) << "    ADC(t-1)=" << adc_tm1
                     << " ADC/TOT=" << main_sample
                     << " TOA=" << toa;

    ch->from_unpacked(Tc, Tp, adc_tm1, main_sample, toa);

    if (bits_left_in_current_word == 0) {
      length++;
      bits_left_in_current_word = 32;
    }
  }

  // the link sub-packets are 32b-word-aligned, so if we are in the middle
  // of a word, we increase the length by one to complete the 32b word
  if (bits_left_in_current_word < 32) {
    length++;
  }

  pflib_log(trace) << "eRx final length " << length;
  return length;
}

ECONDEventPacket::ECONDEventPacket(std::size_t n_links) : links(n_links) {}

void ECONDEventPacket::from(std::span<uint32_t> data) {
  pflib_log(trace) << "econd header one: " << hex(data[0]);
  uint32_t header_marker = ((data[0] >> 23) & mask<9>);
  // two options for header marker: ECOND Spec default and common CMS configuration
  corruption[0] = (header_marker != (0xaa << 1)) and (header_marker != (0xf3 << 1));
  if (corruption[0]) {
    pflib_log(warn) << "Bad header marker from ECOND "
      << "0x" << std::hex << std::setw(2) << std::setfill('0') << (header_marker >> 1)
      << " + 0b" << (header_marker & 0b1);
  }

  uint32_t length = ((data[0] >> 14) & mask<9>);
  pflib_log(trace) << "    length = " << length;
  corruption[1] = (data.size() - 2 != length);
  if (corruption[1]) {
    pflib_log(warn) << "Incomplete event packet, stored payload length "
      << length << " does not equal packet length " << data.size() - 2;
  }

  bool passthrough = ((data[0] >> 13) & mask<1>) == 1;
  bool expected = ((data[0] >> 12) & mask<1>) == 1;
  uint32_t ht_ebo = ((data[0] >> 8) & mask<5>);
  bool m = ((data[0] >> 7) & mask<1>) == 1;
  bool truncated = ((data[0] >> 6) & mask<1>) == 1;
  uint32_t hamming = (data[0] & mask<6>);
  pflib_log(trace) << "    P=" << passthrough << " E=" << expected;

  pflib_log(trace) << "econd header two: " << hex(data[1]);
  uint32_t bx = ((data[1] >> 20) & mask<12>);
  uint32_t l1a = ((data[1] >> 14) & mask<6>);
  uint32_t orb = ((data[1] >> 11) & mask<3>);
  bool subpacket_error = ((data[1] >> 10) & mask<1>) == 1;
  uint32_t rr = ((data[1] >> 8) & mask<2>);
  uint8_t crc = (data[1] & mask<8>);
  pflib_log(trace) << "    BX=" << bx << " L1A=" << l1a << " Orb=" << orb;

  // event header 8-bit CRC
  // uses 8 leading zeros and zeroed Hamming so it is independent from Hamming
  // first header word:
  //   shift out the Hamming
  uint64_t header_crc_base = (data[0] >> 5);
  //   move into position
  header_crc_base <<= 29;
  // second header word, shift out the CRC
  header_crc_base |= (data[1] >> 8);

  pflib_log(trace) << "Header for Calculating 8b CRC: " << hex(header_crc_base);

  uint8_t header_crc_val = utility::econd_crc8(header_crc_base);
  corruption[2] = (header_crc_val != crc);
  if (corruption[2]) {
    pflib_log(warn) << "Event header 8b CRC does not match trasmitted value: "
      << std::bitset<8>(header_crc_val) << " != " << std::bitset<8>(crc);
  }

  if (truncated) {
    pflib_log(warn) << "Buffer control detected insufficient space so it did not transfer link packets.";
    return;
  }

  std::size_t offset{2};
  for (auto& link : links) {
    offset += unpack_link_subpacket(data.subspan(offset), link, passthrough);
    link.bx = bx;
    link.event = l1a;
    link.orbit = orb;
  }

  // offset is now the index of the trailer
  pflib_log(trace) << "Index of trailer: " << offset;

  // the next word is the CRC for all link sub-packets, but not the event packet header
  uint32_t crc_val = utility::crc32(data.subspan(2, offset-2));
  uint32_t target = data[offset];
  corruption[3] = (crc_val != target);
  if (corruption[3]) {
    pflib_log(warn) << "CRC over all link sub-packets does not match transmitted value "
      << hex(crc_val) << " != " << hex(target);
  }
}

Reader& ECONDEventPacket::read(Reader& r) {
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

  pflib_log(trace) << "scanning for end of packet...";
  while (word != 0x12345678) {
    if (!(r >> word)) break;
    pflib_log(trace) << hex(word);
  }

  return r;
}

const std::string ECONDEventPacket::to_csv_header = "i_link,bx,event,orbit,channel,Tp,Tc,adc_tm1,adc,tot,toa";

void ECONDEventPacket::to_csv(std::ofstream &f) const {
  /**
   * The columns of the output CSV are
   * ```
   * i_link, bx, event, orbit, channel, Tp, Tc, adc_tm1, adc, tot, toa
   * ```
   *
   * Since there are 36 channels and one calib channel per DAQ links,
   * there are N*(36+1) rows written for each call to this function
   * where N is the number of links.
   *
   * The trigger links are entirely ignored.
   */
  for (std::size_t i_link{0}; i_link < links.size(); i_link++) {
    const auto& daq_link{links[i_link]};
    f << i_link << ',' << daq_link.bx << ',' << daq_link.event << ','
      << daq_link.orbit << ',' << "calib,";
    daq_link.calib.to_csv(f);
    f << '\n';
    for (std::size_t i_ch{0}; i_ch < 36; i_ch++) {
      f << i_link << ',' << daq_link.bx << ',' << daq_link.event << ','
        << daq_link.orbit << ',' << i_ch << ',';
      daq_link.channels[i_ch].to_csv(f);
      f << '\n';
    }
  }
}

}
