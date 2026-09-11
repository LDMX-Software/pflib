#pragma once
#ifndef PFLIB_TEMPPARAMETERS_H
#define PFLIB_TEMPPARAMETERS_H

namespace pflib {

/**
 * temporarily apply parameters to a chip and then unset them
 *
 * We apply the parameters to the chip in the constructor and then unset
 * them in the destructor.
 * For example
 * ```cpp
 * // before, PAGE.PARAM = 1
 * {
 *   TempParameters<ROC> temp_param_lock{roc, {"PAGE", {"PARAM", 3}}};
 *   // here, PAGE.PARAM = 3
 * }
 * // after temp_param_lock is destructed, PAGE.PARAM = 1 again
 * ```
 *
 * This class also has a `Builder` sub-class which can be helpful
 * for constructing a set of parameters.
 */
template <class Chip>
class TempParameters {
 public:
  /// applies the new parameters and holds the previous registers
  TempParameters(
      Chip& chip,
      const std::map<std::string, std::map<std::string, uint64_t>>& parameters)
      : chip_{chip} {
    previous_registers_ = chip_.applyParameters(parameters);
  }
  /// applies the previous registers back onto the chip
  ~TempParameters() { chip_.setRegisters(previous_registers_); }
  /// cannot copy or assign this lock
  TempParameters(const TempParameters&) = delete;
  TempParameters& operator=(const TempParameters&) = delete;
  /// Build a TempParameters class parameter by parameters
  class Builder {
    std::map<std::string, std::map<std::string, uint64_t>> parameters_;
    Chip& chip_;

   public:
    Builder(Chip& chip) : parameters_{}, chip_{chip} {}
    Builder& add(const std::string& page, const std::string& param,
                 const uint64_t& val) {
      parameters_[page][param] = val;
      return *this;
    }
    [[nodiscard]] TempParameters apply() {
      return TempParameters(chip_, parameters_);
    }
  };

 private:
  /// handle to a Chip that has setRegisters
  Chip& chip_;
  /// values for previous registers that were touched upon construction
  std::map<int, std::map<int, uint8_t>> previous_registers_;
};

}  // namespace pflib

#endif
