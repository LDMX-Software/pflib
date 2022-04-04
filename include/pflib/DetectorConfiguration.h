#ifndef PFLIB_DETECTORCONFIGURATION_H
#define PFLIB_DETECTORCONFIGURATION_H

#include <string>
#include <map>

namespace YAML {
class Node;
}

namespace pflib {

/**
 * object for parsing a detector configuration file and
 * (potentially) executing it
 */
class DetectorConfiguration {
  struct PolarfireConfiguration {
    int calib_offset_; // FC.CALIB
    int sipm_bias_; 
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
