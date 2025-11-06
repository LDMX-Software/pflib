#include "pflib/Parameters.h"

#include <yaml-cpp/yaml.h>

namespace pflib {

/**
 * no easy way to do it, so we just try the three that we care about.
 */
std::any extract_scalar(const YAML::Node& node) {
  try {
    return node.as<int>();
  } catch (const YAML::TypedBadConversion<int>&) {
    try {
      return node.as<double>();
    } catch (const YAML::TypedBadConversion<double>&) {
      try {
        return node.as<std::string>();
      } catch (const YAML::TypedBadConversion<std::string>&) {
        PFEXCEPTION_RAISE("BadType", "Unrecognized YAML::Scalar type.");
      }
    }
  }
}

std::any extract_sequence(const YAML::Node& node) {
  try {
    return node.as<std::vector<int>>();
  } catch (const YAML::TypedBadConversion<std::vector<int>>&) {
    try {
      return node.as<std::vector<double>>();
    } catch (const YAML::TypedBadConversion<std::vector<double>>&) {
      try {
        return node.as<std::vector<std::string>>();
      } catch (const YAML::TypedBadConversion<std::vector<std::string>>&) {
        PFEXCEPTION_RAISE("BadType", "Unrecognized YAML::Scalar type.");
      }
    }
  }
}

// Null, Scalar, Sequence, Map
void extract(const YAML::Node& yaml_config, Parameters& deduced_config) {
  if (not yaml_config.IsMap()) {
    // if there isn't a map, then this is not a YAML file providing configuration parameters
    PFEXCEPTION_RAISE("BadFile", "YAML file specifying parameters needs to be a map.");
  }

  for (YAML::const_iterator it = yaml_config.begin(); it != yaml_config.end(); it++) {
    auto key = it->first;
    auto val = it->second;
    if (not key.IsScalar()) {
      // key needs to be a scalar
      PFEXCEPTION_RAISE("BadSchema", "The Key in a YAML Map is somehow not a scalar.");
    }
    std::string keyname = key.as<std::string>();
    if (val.Type() == YAML::NodeType::Scalar) {
      deduced_config.add(keyname, extract_scalar(val));
    } else if (val.Type() == YAML::NodeType::Sequence) {
      deduced_config.add(keyname, extract_sequence(val));
    } else if (val.Type() == YAML::NodeType::Map) {
      // recursion
      Parameters sub_parameters;
      extract(val, sub_parameters);
      deduced_config.add(keyname, sub_parameters);
    } else {
      // Null or something else
      // silently skip to allow for comments
      break;
    }
  }
}

Parameters Parameters::from_yaml(const std::string& filepath) {
  YAML::Node config;
  try {
    config = YAML::LoadFile(filepath);
  } catch (const YAML::BadFile& e) {
    PFEXCEPTION_RAISE("BadFile", "Unable to load file " + filepath);
  }

  Parameters p;
  extract(config, p);
  return p;
}

}
