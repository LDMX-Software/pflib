/**
 * @file register_maps_types.h
 * Header defining types stored within the parameter to register mappings
 *
 * The specific maps written into the headers corresponding
 * to the names of the HGCROC type/versions rely on this header
 * to join them together into a coherent interface for the compiler
 * and to include the necessary headers they rely upon.
 */

#pragma once

// need maps and strings for the LUTs
#include <map>
#include <string>
#include <vector>

/**
 * Structure holding a location in the registers
 */
struct RegisterLocation {
  /// the register the parameter is in (0-31)
  const int reg;
  /// the min bit the location is in the register (0-7)
  const int min_bit;
  /// the number of bits the location has in the register (1-8)
  const int n_bits;
  /// the mask for this number of bits
  const int mask;
  /**
   * Usable constructor
   *
   * This constructor allows us to calculate the mask from the
   * number of bits so that the downstream compilation functions
   * don't have to calculate it themselves.
   */
  RegisterLocation(int r, int m, int n)
      : reg{r}, min_bit{m}, n_bits{n}, mask{((1 << n_bits) - 1)} {}
};

/**
 * A parameter for the HGC ROC includes one or more register locations
 * and a default value defined in the manual.
 */
struct Parameter {
  /// the default parameter value
  const uint64_t def;
  /// the locations that the parameter is split over
  const std::vector<RegisterLocation> registers;
  /// pass locations and default value of parameter
  Parameter(std::initializer_list<RegisterLocation> r, uint64_t def)
      : def{def}, registers{r} {}
  /// short constructor for single-location parameters
  Parameter(int r, int m, int n, uint64_t def)
      : Parameter({RegisterLocation(r, m, n)}, def) {}
};

/**
 * A direct access parameter is used to directly configure the HGCROC I2C
 * connection in a fast but simplified manner.
 *
 * Direct access is slightly simpler, none of the direct access parameters
 * spread across more than one register and are all only one bit. The registers
 * that the direct access parameters are in are also different than the
 * registers that the normal configuration parameters are in. They are situated
 * within the I2C circuit and so their register is relative to the root I2C
 * address of the HGCROC.
 */
struct DirectAccessParameter {
  /// the register this parameter is in (relative to the root I2C address of the
  /// HGCROC) (4-7)
  const int reg;
  /// the bit location within the register that this parameter is in (0-7)
  const int bit_location;
  /// the default parameter value
  const bool def;
};

/**
 * A non-copyable mapping
 *
 * This is helpful for our Look Up Tables (LUTs) since we want
 * to define them once and then just pass them around without
 * copying their large size.
 *
 * Always retrieve constant references of these types, for example
 * ```cpp
 * const auto& my_handle = get_lut();
 * ```
 */
template <typename Key, typename Val>
class NoCopyMap : public std::map<Key, Val> {
 public:
  using Mapping = std::map<Key, Val>;
  NoCopyMap(const NoCopyMap&) = delete;
  NoCopyMap& operator=(const NoCopyMap&) = delete;
  /**
   * Constructor necessary to support a simpler definition syntax.
   * ```cpp
   * using MyLUT = NoCopyMap<Key,Val>;
   * const MyLUT EXAMPLE = NoCopyMap::Mapping({
   *   {key, val},
   *   // etc...
   * });
   * ```
   * I tried playing around with std::initializer_list but I
   * couldn't get it to work and since the headers where these
   * LUTs are defined are written by a script anyways, I decided
   * to leave the boilerplate in.
   */
  NoCopyMap(const Mapping& contents) : Mapping(contents) {}
};

/// type for hold sets of parameters by name
using Page = NoCopyMap<std::string, Parameter>;

/// type for holding a set of abstract pages just associating names with
/// specific sets of parameters
using PageLUT = NoCopyMap<std::string, const Page&>;

/// type for a LUT that holds concrete pages with their address and Page
/// parameters
using ParameterLUT = NoCopyMap<std::string, std::pair<int, const Page&>>;

/// direct access parameters LUT
using DirectAccessParameterLUT = NoCopyMap<std::string, DirectAccessParameter>;
