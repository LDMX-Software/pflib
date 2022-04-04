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

class PolarfireSetting {
 public:
  PolarfireSetting() = default;
  virtual ~PolarfireSetting() = default;
  virtual void import(YAML::Node val) = 0;
  virtual void execute(PolarfireTarget* pft) = 0;
  virtual void stream(std::ostream& s) = 0;
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
  
    /// private constructor to prevent creation
    Factory() = default;
  
    /// library of possible objects to create
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
   * we need access to all the WB, so make sure no other instances of pftool (for example) are up!
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
