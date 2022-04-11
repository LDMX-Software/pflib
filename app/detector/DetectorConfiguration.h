#ifndef PFLIB_DETECTORCONFIGURATION_H
#define PFLIB_DETECTORCONFIGURATION_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <functional>

#include "pflib/Exception.h"

namespace YAML {
class Node;
}

namespace pflib {
class PolarfireTarget;
namespace detail {

/**
 * Object that can change a setting on a polarfire target
 *
 * This is a "prototype" that is used as the base pointer for
 * derived classes which can do arbitrary tasks to the passed
 * PolarfireTarget
 */
class PolarfireSetting {
 public:
  PolarfireSetting() = default;
  virtual ~PolarfireSetting() = default;
  /// import the setting from the passes yaml node
  virtual void import(YAML::Node val) = 0;
  /// apply the setting to the passed polarfire target
  virtual void execute(PolarfireTarget* pft) = 0;
  /// print the setting
  virtual void stream(std::ostream& s) = 0;
  /**
   * The object that can construct new polarfire settings given
   * the name of the setting
   */
  class Factory {
   public:
    static Factory& get() {
      static Factory the_factory;
      return the_factory;
    }
    template<typename DerivedType>
    uint64_t declare(const std::string& name) {
      library_[name] = &maker<DerivedType>;
      return reinterpret_cast<std::uintptr_t>(&library_);
    }
    std::unique_ptr<PolarfireSetting> make(const std::string& full_name) {
      auto lib_it{library_.find(full_name)};
      if (lib_it == library_.end()) {
        PFEXCEPTION_RAISE("BadFormat", "Unrecognized setting "+full_name);
      }
      return lib_it->second();
    }
    Factory(Factory const&) = delete;
    void operator=(Factory const&) = delete;
   private:
    template <typename DerivedType>
    static std::unique_ptr<PolarfireSetting> maker() {
      return std::make_unique<DerivedType>();
    }
    Factory() = default;
    std::map<std::string, std::function<std::unique_ptr<PolarfireSetting>()>> library_;
  };  // Factory
};  // PolarfireSetting
}  // namespace detail

/**
 * object for parsing a detector configuration file and
 * (potentially) executing it
 */
class DetectorConfiguration {
  struct PolarfireConfiguration {
    std::map<std::string,
      std::unique_ptr<detail::PolarfireSetting>> settings_;
    std::map<int, // roc index on this polarfire
      std::map<
        std::string, // page
        std::map<
          std::string, // parameter
          int // value
      >>> hgcrocs_;
    void import(YAML::Node conf);
    void apply(const std::string& host);
  };
  std::map<std::string, PolarfireConfiguration> polarfires_;
 public:
  /**
   * Parse the input file loading into us
   */
  DetectorConfiguration(const std::string& config);

  /**
   * apply the configuration to the detector,
   * we need access to all the wishbones, 
   * so make sure no other instances of pftool (for example) are up!
   */
  void apply();

  /**
   * Print out the detector configuration for debugging purposes
   */
  void stream(std::ostream& s) const;
};
}

std::ostream& operator<<(std::ostream& s, const pflib::DetectorConfiguration& dc);

#endif
