#pragma once
#ifndef TOOL_SPACESEPARATED_H
#define TOOL_SPACESEPARATED_H

/**
 * emit an iterator-range as a space-separated list
 */
template <typename Iterator>
class SpaceSeparated {
  Iterator begin_;
  Iterator end_;

 public:
  SpaceSeparated(Iterator begin, Iterator end) : begin_{begin}, end_{end} {}
  friend inline std::ostream& operator<<(std::ostream& o,
                                         const SpaceSeparated& self) {
    o << "[";
    for (auto it{self.begin_}; it != self.end_; it++) {
      o << " " << *it;
    }
    o << " ]";
    return o;
  }
};

#endif
