#include "pflib/packing/ECONDEventPacket.h"

#include "pflib/packing/Hex.h"
#include "pflib/packing/Mask.h"

#include <iostream>

namespace pflib::packing {

void ECONDEventPacket::LinkSubPacket::from(std::span<uint32_t> data) {
  pflib_log(trace) << "link header " << hex(data[0]);
  // sub-packet start
  uint32_t stat = ((data[0] >> 29) & mask<3>);
  corruption[0] = (stat != 0b111); // all ones is GOOD
  if (corruption[0]) {
    pflib_log(warn) << "bad subpacket checks";
  }
  uint32_t ham = ((data[0] >> 26) & mask<3>);
  bool is_empty = ((data[0] >> 25) & mask<1>) == 1;
  uint32_t cm0 = ((data[0] >> 15) & mask<12>);
  uint32_t cm1 = ((data[0] >>  5) & mask<12>);
  pflib_log(trace) << "    is_empty=" << is_empty
                   << " CM0=" << cm0
                   << " CM1=" << cm1;
  if (is_empty) {
    // F=1
    // single-word header, no channel data or channel map, entire link is empty
    length = 1;
    bool error_caused_empty = ((data[0] >> 4) & mask<1>);
    // E=0 (false) means none of the 37 channels passed zero suppression
    // E=1 (true) means the unmasked Stat error bits caused the sub-packet to be suppressed
    pflib_log(trace) << "is empty, E=" << error_caused_empty;
  } else {
    pflib_log(trace) << "chan map lower 32 " << hex(data[1]);
    // construct channel map
    channel_map = data[1];
    channel_map |= ((data[0] & mask<5>) << 32);
    pflib_log(trace) << "is not empty, channel_map=" << channel_map;

    // start length with two header words (including channel map)
    length = 2;

    // consume channel data
    int bits_left_in_current_word{32};
    for (std::size_t i_chan{0}; i_chan < 37; i_chan++) {
      pflib_log(trace) << "length=" << length << " blic=" << bits_left_in_current_word << " i_ch=" << i_chan;
      // skip channels that are not passed
      if (not channel_map[i_chan]) {
        pflib_log(trace) << "skipping ch " << i_chan << " because its not in map";
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
      if (code == 0b0000) {
        // TOA ZS
        code_len = 4;
        has_adctm1 = true;
        has_toa = false;
        extra = 0;
        nbits = 24;
      } else if (code == 0b0001) {
        // ADC(-1) and TOA ZS
        code_len = 4;
        has_adctm1 = false;
        has_toa = false;
        extra = 2;
        nbits = 16;
      } else if (code == 0b0010) {
        // No ZS or ...
        code_len = 4;
        has_adctm1 = true;
        has_toa = false;
        extra = 0;
        nbits = 24;
      } else if (code == 0b0011) {
        // ADC(-1) ZS
        code_len = 4;
        has_adctm1 = false;
        has_toa = true;
        extra = 0;
        nbits = 24;
      } else if ((code >> 2) == 0b01) {
        // full pass ZS in ADC mode
        code_len = 2;
        has_adctm1 = true;
        has_toa = true;
        extra = 0;
        nbits = 32;
      } else if ((code >> 2) == 0b11) {
        // pass ZS because in TOT mode
        code_len = 2;
        has_adctm1 = true;
        has_toa = true;
        extra = 0;
        nbits = 32;
      } else if ((code >> 2) == 0b10) {
        // invalid but known code
        pflib_log(warn) << "ECOND eRx invalid code " << std::bitset<4>(code);
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
        chan_data = ((data[length] & ((1ul << bits_left_in_current_word) - 1ul)) << (nbits - bits_left_in_current_word));
        length++;
        bits_left_in_current_word = 32;
        chan_data += ((data[length] >> runon_length) & ((1ul << runon_length) - 1ul));
        bits_left_in_current_word -= runon_length;
      } else {
        chan_data = ((data[length] >> (bits_left_in_current_word - nbits)) & ((1ul << nbits)-1ul));
        bits_left_in_current_word -= nbits;
      }

      pflib_log(trace) << "    chan_data=" << hex(chan_data);

      // unpack the channel data into the measurements that it contains
      // none of these measurements can be negative, so a negative value
      // corresponds to an measurement that did not exist
      int adc_tm1{-1}, adc{-1}, tot{-1}, toa{-1};

      // shift out padding extra bits
      chan_data >>= extra;

      // next lowest 10 bits are TOA if it has it
      if (has_toa) {
        toa = (chan_data & mask<10>);
        chan_data >>= 10;
      }

      // next lowest 10 bits are the main sample ADC or TOT
      uint32_t sample = (chan_data & mask<10>);
      chan_data >>= 10;
      if (code_len == 2 and (code >> 2) == 0b11) {
        tot = sample;
      } else {
        adc = sample;
      }

      // next lowest 10 bits are the ADCt-1 if it has it
      if (has_adctm1) {
        adc_tm1 = (chan_data & mask<10>);
      }

      pflib_log(trace) << "    ADC(t-1)=" << adc_tm1
                       << " ADC=" << adc
                       << " TOT=" << tot
                       << " TOA=" << toa;
      channel_data.emplace_back(i_chan, chan_data);

      if (bits_left_in_current_word == 0) {
        length++;
        bits_left_in_current_word = 32;
      }
    }
    length++;

    pflib_log(trace) << "eRx final length " << length;
  }
}

ECONDEventPacket::ECONDEventPacket(std::size_t n_links) : link_subpackets(n_links) {}

void ECONDEventPacket::from(std::span<uint32_t> data) {
  pflib_log(trace) << "econd header one: " << hex(data[0]);
  uint32_t header_marker = ((data[0] >> 23) & mask<9>);
  corruption[0] = (header_marker != (0xaa << 1));
  if (corruption[0]) {
    pflib_log(warn) << "Bad header marker from ECOND " << hex(header_marker);
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
  uint32_t ht_ebo = ((data[0] >> 8) & mask<8>);
  bool m = ((data[0] >> 7) & mask<1>) == 1;
  bool truncated = ((data[0] >> 6) & mask<1>) == 1;
  uint32_t hamming = (data[0] & mask<6>);
  pflib_log(trace) << "    P=" << passthrough << " E=" << expected;
  // ignoring other checks right now...

  pflib_log(trace) << "econd header two: " << hex(data[1]);
  uint32_t bx = ((data[1] >> 20) & mask<12>);
  uint32_t l1a = ((data[1] >> 14) & mask<6>);
  uint32_t orb = ((data[1] >> 11) & mask<3>);
  bool subpacket_error = ((data[1] >> 10) & mask<1>) == 1;
  uint32_t rr = ((data[1] >> 8) & mask<2>);
  uint32_t crc = (data[1] & mask<8>);
  pflib_log(trace) << "    BX=" << bx << " L1A=" << l1a << " Orb=" << orb;

  std::size_t offset{2};
  for (auto& link_subpacket : link_subpackets) {
    link_subpacket.from(data.subspan(offset));
    offset += link_subpacket.length;
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

}
