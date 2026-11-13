#pragma once
#ifndef PFLIB_PACKING_ECONTALIGNMENTWORD_H
#define PFLIB_PACKING_ECONTALIGNMENTWORD_H

#include <cstdint>
#include <ostream>

namespace pflib::packing {

/**
 * The ECON-T alignment outputs a fixed pattern consisting of two 16-bit words.
 *
 * See Section 20 of the ECON-T manual.
 *
 * The two 16-bit words look like
 *  5b BX | 11b Pattern
 * where BX is a BX counter and the 11b pattern is set by
 * FormatterBUffer.global.link_reset_pattern.
 */
struct ECONTAlignmentWord {
  uint32_t word_;
  uint32_t bx0() const;
  uint32_t bx1() const;
  uint32_t patt0() const;
  uint32_t patt1() const;
  void from(uint32_t bx, uint32_t patt);
  ECONTAlignmentWord();
  explicit ECONTAlignmentWord(uint32_t w);
  /**
   * A valid match is where both BX stored in the two 16-bit words match
   * and both 11b patterns match the passed in value.
   */
  bool match(uint32_t pattern) const;
  friend std::ostream& operator<<(std::ostream& o, const ECONTAlignmentWord& self);
};

}

#endif
