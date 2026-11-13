#include "pflib/packing/ECONTAlignmentWord.h"

#include <iomanip>

namespace pflib::packing {

uint32_t ECONTAlignmentWord::bx0() const {
  return (word_ >> 11) & 0x1f;
}

uint32_t ECONTAlignmentWord::bx1() const {
  return (word_ >> (16+11)) & 0x1f;
}

uint32_t ECONTAlignmentWord::patt0() const {
  return (word_ & 0x7ff);
}

uint32_t ECONTAlignmentWord::patt1() const {
  return ((word_ >> 16) & 0x7ff);
}

void ECONTAlignmentWord::from(uint32_t bx, uint32_t patt) {
  uint32_t half_word = (((bx & 0x1f) << 11) | (patt & 0x7ff));
  word_ = (half_word << 16) | half_word;
}

ECONTAlignmentWord::ECONTAlignmentWord() : word_{0} {}
ECONTAlignmentWord::ECONTAlignmentWord(uint32_t w) : word_{w} {}

bool ECONTAlignmentWord::match(uint32_t pattern) const {
  return (bx0() == bx1()) and (patt0() == pattern) and (patt1() == pattern);
}

std::ostream& operator<<(std::ostream& o, const ECONTAlignmentWord& self) {
  o << std::dec << std::setw(2) << self.bx1() << " "
    << std::hex << std::setw(3) << self.patt1() << " "
    << std::dec << std::setw(2) << self.bx0() << " "
    << std::hex << std::setw(3) << self.patt0()
    << std::dec << std::setw(0);
  return o;
}
}
