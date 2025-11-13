#include "pflib/Parameters.h"

#include <filesystem>
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
  } catch (const YAML::TypedBadConversion<int>&) {
    try {
      return node.as<std::vector<double>>();
    } catch (const YAML::TypedBadConversion<double>&) {
      try {
        return node.as<std::vector<std::string>>();
      } catch (const YAML::TypedBadConversion<std::string>&) {
        PFEXCEPTION_RAISE("BadType", "Unrecognized content of a YAML Sequence.");
      }
    }
  }
}

// Null, Scalar, Sequence, Map
void extract(std::filesystem::path config_dir, const YAML::Node& yaml_config,
             Parameters& deduced_config, bool overlay) {
  if (not yaml_config.IsMap()) {
    // if there isn't a map, then this is not a YAML file providing
    // configuration parameters
    PFEXCEPTION_RAISE("BadFile",
                      "YAML file specifying parameters needs to be a map.");
  }

  for (YAML::const_iterator it = yaml_config.begin(); it != yaml_config.end();
       it++) {
    auto key = it->first;
    auto val = it->second;
    if (not key.IsScalar()) {
      // key needs to be a scalar
      PFEXCEPTION_RAISE("BadSchema",
                        "The Key in a YAML Map is somehow not a scalar.");
    }
    std::string keyname = key.as<std::string>();
    if (val.Tag() == "!file") {
      auto filepath = val.as<std::string>();
      if (filepath.empty()) {
        PFEXCEPTION_RAISE("BadFilePath",
                          "Key named "+keyname+" is tagged as another file but is empty.");
      }
      Parameters sub_parameters;
      std::filesystem::path fp{filepath};
      if (fp.is_absolute()) {
        // assume root path
        sub_parameters.from_yaml(fp, overlay);
      } else {
        // relative to current directory
        sub_parameters.from_yaml(config_dir / fp, overlay);
      }

      deduced_config.add(keyname, sub_parameters, overlay);
    } else if (val.Type() == YAML::NodeType::Scalar) {
      deduced_config.add(keyname, extract_scalar(val), overlay);
    } else if (val.Type() == YAML::NodeType::Sequence) {
      deduced_config.add(keyname, extract_sequence(val), overlay);
    } else if (val.Type() == YAML::NodeType::Map) {
      // recursion
      Parameters sub_parameters;
      extract(config_dir, val, sub_parameters, overlay);
      deduced_config.add(keyname, sub_parameters, overlay);
    } else {
      // Null or something else
      // silently skip to allow for comments
      break;
    }
  }
}

bool Parameters::exists(const std::string& name) const {
  return parameters_.find(name) != parameters_.end();
}

std::vector<std::string> Parameters::keys() const {
  std::vector<std::string> key;
  for (auto i : parameters_) key.push_back(i.first);
  return key;
}

void Parameters::from_yaml(const std::string& filepath, bool overlay) {
  YAML::Node config;
  try {
    config = YAML::LoadFile(filepath);
  } catch (const YAML::BadFile& e) {
    PFEXCEPTION_RAISE("BadFile", "Unable to load file " + filepath);
  }

  std::filesystem::path fp{filepath};

  extract(fp.parent_path(), config, *this, overlay);
}

}  // namespace pflib
