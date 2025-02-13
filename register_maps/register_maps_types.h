/* unifying header for all register maps
 * the specific maps written into the headers corresponding
 * to the names of the HGCROC type/versions rely on this header
 * to join them together into a coherent interface for the compiler
 * and to include the necessary headers they rely upon.
 */

#pragma once

// need maps and strings for the LUTs
#include <map>
#include <string>

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
    : reg{r}, min_bit{m}, n_bits{n},
      mask{((1 << n_bits) - 1)} {}
};

/**
 * A parameter for the HGC ROC includes one or more register locations
 * and a default value defined in the manual.
 */
struct Parameter {
  /// the default parameter value
  const int def;
  /// the locations that the parameter is split over
  const std::vector<RegisterLocation> registers;
  /// pass locations and default value of parameter
  Parameter(std::initializer_list<RegisterLocation> r,int def)
    : def{def}, registers{r} {}
  /// short constructor for single-location parameters
  Parameter(int r, int m, int n, int def)
    : Parameter({RegisterLocation(r,m,n)},def) {}
};

// type for a page specifying names and specifications of parameters
using Page = std::map<std::string, Parameter>;

// type for holding a set of abstract pages just associating names with specific sets of parameters
using PageLUT = std::map<std::string, const Page&>;

// type for a LUT that holds concrete pages with their address and Page parameters
using ParameterLUT = std::map<std::string, std::pair<int, const Page&>>;

