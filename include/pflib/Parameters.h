#pragma once
#ifndef PFLIB_CONFIG_PARAMETERS_H
#define PFLIB_CONFIG_PARAMETERS_H

/*~~~~~~~~~~~~~~~~*/
/*   C++ StdLib   */
/*~~~~~~~~~~~~~~~~*/
#include <any>
#include <boost/core/demangle.hpp>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <vector>

#include "pflib/Exception.h"

namespace pflib {

/**
 * Class encapsulating parameters for generically holding configuration
 * parameters
 *
 * The storage of arbitrary parameters recursively is done using a map
 * from parameter names (std::string) to parameter values. The values
 * are stored in std::any which allows the user to store any object they wish
 * in a memory-safe environment.
 */
class Parameters {
 public:
  /**
   * Add a parameter to the parameter list.  If the parameter already
   * exists in the list, throw an exception.
   *
   * @param[in] name Name of the parameter.
   * @param[in] value The value of the parameter.
   * @tparam T type of parameter being added
   * @throw Exception if a parameter by that name already exist in
   *  the list.
   */
  template <typename T>
  void add(const std::string& name, const T& value, bool overlay = false) {
    if (not overlay and exists(name)) {
      PFEXCEPTION_RAISE("Config",
                        "The parameter " + name +
                            " already exists in the list of parameters.");
    }

    parameters_[name] = value;
  }

  /**
   * Check to see if a parameter exists
   *
   * @param[in] name name of parameter to check
   * @return true if parameter exists in configuration set
   */
  bool exists(const std::string& name) const;

  /**
   * Retrieve the parameter of the given name.
   *
   * @throw Exception if parameter of the given name isn't found
   * @throw Exception if parameter is found but not of the input type
   *
   * @tparam T the data type to cast the parameter to.
   * @param[in] name the name of the parameter value to retrieve.
   * @return The user specified parameter of type T.
   */
  template <typename T>
  const T& get(const std::string& name) const {
    // Check if the variable exists in the map.  If it doesn't,
    // raise an exception.
    if (not exists(name)) {
      PFEXCEPTION_RAISE(
          "Config",
          "Parameter '" + name + "' does not exist in list of parameters.");
    }

    try {
      return std::any_cast<const T&>(parameters_.at(name));
    } catch (const std::bad_any_cast& e) {
      PFEXCEPTION_RAISE(
          "Config",
          "Parameter '" + name + "' of type '" +
              boost::core::demangle(parameters_.at(name).type().name()) +
              "' is being cast to incorrect type '" +
              boost::core::demangle(typeid(T).name()) + "'.");
    }
  }

  /**
   * Retrieve a parameter with a default specified.
   *
   * Return the input default if a parameter is not found in map.
   *
   * @tparam T type of parameter to return
   * @param[in] name parameter name to retrieve
   * @param[in,out] def default to use if parameter is not found
   * @return the user parameter of type T
   */
  template <typename T>
  const T& get(const std::string& name, const T& def) const {
    if (not exists(name)) return def;

    // get here knowing that name exists in parameters_
    return get<T>(name);
  }

  /**
   * Get a list of the keys available.
   *
   * This may be helpful in debugging to make sure the parameters are spelled
   * correctly.
   *
   * @return list of all keys in this map of parameters
   */
  std::vector<std::string> keys() const;

  /**
   * Load parameters from a YAML file
   *
   * @param[in] filepath path to YAML file to load into us
   * @param[in] overlay allow the input YAML to overwrite current parameters
   */
  void from_yaml(const std::string& filepath, bool overlay = true);

 private:
  /// container holding parameter names and their values
  std::map<std::string, std::any> parameters_;

};  // Parameters
}  // namespace pflib

#endif  // PFLIB_CONFIG_PARAMETERS_H
