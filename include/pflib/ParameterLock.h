#pragma once

namespace pflib {

class ParameterLock {
  std::map<std::string, std::map<std::string, int>> unset_;
  ROC& roc_;
  ParameterLock(std::map<std::string, std::map<std::string, int>>& unset, ROC& roc)
    : unset_{unset}, roc_{roc} {}
 public:
  ~ParameterLock() {
    roc_.applyParameters(unset_);
  }
  class Builder {
    std::map<std::string, std::map<std::string, int>> unset_{}, set_{};
    ROC& roc_;
   public:
    Builder(ROC& roc) : unset_{}, set_{}, roc_{roc} {}
    Builder& set(const std::string& page, const std::string& param, int val, int unset = 0) {
      set_[page][param] = val;
      unset_[page][param] = unset;
      return *this;
    }
    ParameterLock lock() {
      roc_.applyParameters(set_);
      return ParameterLock(unset_, roc_);
    }
  };
};

}
