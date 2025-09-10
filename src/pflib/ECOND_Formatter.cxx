#include "pflib/ECOND_Formatter.h"

#include <stdio.h>

namespace pflib {

ECOND_Formatter::ECOND_Formatter() {}

void ECOND_Formatter::startEvent(int bx, int l1a, int orbit) {
  packet_.clear();
  packet_.push_back((0xAA << 24) |
                    (1 << 7));  // header marker, no hamming,  matching
  packet_.push_back(
      ((bx & 0xFFF) << 20) | ((l1a & 0x3F) << 14) |
      ((orbit & 0x7) << 11));  // bx, l1a, orbit, S=0, RR=0, no CRC-8
};

void ECOND_Formatter::add_elink_packet(int ielink,
                                       const std::vector<uint32_t>& src) {
  // format the elink's data
  std::vector<uint32_t> subpacket = format_elink(ielink, src);
  // copy in the formatted data
  packet_.insert(packet_.end(), subpacket.begin(), subpacket.end());
  // update the length
  packet_[0] =
      (packet_[0] & 0xFF803FFFu) | (((packet_.size() - 2 + 1) & 0x1FF) << 14);
}

void ECOND_Formatter::finishEvent() {
  packet_.push_back(0xFFFFFFFFu);  // CRC, not actually...
}

std::vector<uint32_t> ECOND_Formatter::format_elink(
    int ielink, const std::vector<uint32_t>& src) {
  std::vector<uint32_t> dest;
  int n_readout = 0;
  // check for right number of words, correct header, etc
  if (src.size() != 40 || ((src[0] >> 28) & 0xF) != 0xF) {
    //      if (src.size()!=40 || ((src[0]>>28)&0xF)!=0xF || (((src[0]&0xF)!=0x5 && (src[0]&0xF)!=0x2))) {
    printf("Invalid contents\n");
    return dest;
  }
  dest.push_back(0);
  dest.push_back(0);

  // stat bits (assuming happy for now)
  dest[0] |= (0x7u << 29);
  // hamming bits
  dest[0] |= (src[0] & 0x70) << (26 - 4);
  // common mode
  dest[0] |= (src[1] & 0xFFFFF) << 5;

  uint32_t building_word = 0;
  int space_left = 32;  // bits in the word

  for (int iw = 0; iw < 37; iw++) {
    uint32_t word = src[2 + iw];
    int ic = iw;
    if (ic == 18)
      ic = -1;
    else if (ic > 18)
      ic -= 1;
    int code = zs_process(ielink, ic, word);
    if (code >= 0) {
      // set the channel map bit
      if (ic >= 32)
        dest[0] |= (1 << (ic - 32));
      else
        dest[1] |= (1 << ic);

      uint32_t insert_value;
      int insert_len = 32;
      if (code == 0) {  // TOA ZS
        insert_value = 0 | ((word >> 10) & 0xFFFFF);
        insert_len = 24;
      } else if (code == 1) {  // ADC-1 and TOA ZS
        insert_value = (0x1 << 12) | ((word >> 8) & 0xFFC);
        insert_len = 16;
      } else if (code == 2) {  // TcTp=01, TOA ZS
        insert_value = (0x2 << 12) | ((word >> 10) & 0xFFFFF);
        insert_len = 24;
      } else if (code == 3) {  // ADC-1 ZS
        insert_value = (0x3 << 12) | (word & 0xFFFFF);
        insert_len = 24;
      } else if (code == 4) {  // readout all, ADC
        insert_value = (0x1 << 30) | (word & 0x3FFFFFFF);
      } else if (code == 12) {  // readout all, TOT
        insert_value = word;
      } else {  // invalid code, readout all
        insert_value = word;
      }
      //
      while (insert_len >= space_left) {
        building_word |= (insert_value >> (insert_len - space_left));
        if ((insert_len - space_left) == 8)
          insert_value &= 0xFF;
        else if ((insert_len - space_left) == 16)
          insert_value &= 0xFFFF;
        else if ((insert_len - space_left) == 24)
          insert_value &= 0xFFFFFF;
        insert_len -= space_left;
        dest.push_back(building_word);
        building_word = 0;
        space_left = 32;
      }
      if (insert_len > 0) {
        building_word |= insert_value << (space_left - insert_len);
        space_left -= insert_len;
      }
    }
  }
  if (space_left != 32) {
    dest.push_back(building_word);
  }

  return dest;
}
int ECOND_Formatter::zs_process(int ielink, int ic, uint32_t word) {
  // eventually, implement detailed code to carry out different classes of ZS with provided
  // parameters.  For now, we just look at the tc/tp code
  int tctp = (word >> 30) & 0x3;
  if (tctp == 0x3) return 12;  // is TOT
  if (tctp == 0x2) return 8;   // is strange
  if (tctp == 0x1) return 2;   // is invalid due to ongoing TOT
  /// at this point, we have tctp=0, so we can apply zs algorithms
  if (disable_ZS_) return 4;
  // TOA zs...
  bool no_toa = ((word & 0x3FF) == 0);
  if (no_toa) return 0;
  return 4;  // assume we just read out everything for now, but easy to add TOA zs logic
}

}  // namespace pflib
