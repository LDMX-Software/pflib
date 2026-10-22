#pragma once
#ifndef TOOL_SPACESEPARATED_H
#define TOOL_SPACESEPARATED_H

/**
 * emit a vector as a space-separate list
 */
template <typename T>
class SpaceSeparated {
  const std::vector<T>& v_;

 public:
  SpaceSeparated(const std::vector<T>& v) : v_{v} {}
  friend inline std::ostream& operator<<(std::ostream& o,
                                         const SpaceSeparated& self) {
    o << "[";
    for (const auto& v : self.v_) {
      o << " " << v;
    }
    o << " ]";
    return o;
  }
};

#endif
