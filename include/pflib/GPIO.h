#ifndef PFLIB_GPIO_H_
#define PFLIB_GPIO_H_

#include "pflib/WishboneTarget.h"
#include <vector>

namespace pflib {

/**
 * @class GPIO
 * @brief Class which encapsulates the GPIO controller in the Polarfire
 */
class GPIO : public WishboneTarget {
 public:
  GPIO(WishboneInterface* wb, int target = tgt_GPIO) : WishboneTarget(wb,target) {
    ngpi_=-1;
    ngpo_=-1;
  }

  /**
   * Get the number of GPO bits 
   */
  int getGPOcount();

  /**
   * Get the number of GPI bits 
   */
  int getGPIcount();

  /** 
   * Read a GPI bit
   */
  bool getGPI(int ibit);

  /** 
   * Read all GPI bits
   */
  std::vector<bool> getGPI();

  /** 
   * Set a single GPO bit
   */
  void setGPO(int ibit, bool toTrue);

  /**
   * Set all GPO bits
   */
  void setGPO(const std::vector<bool>& bits);

  /** 
   * Read all GPO bits
   */
  std::vector<bool> getGPO();

  /// convenience wrapper for python bindings
  bool getGPI_single(int ibit) { return getGPI(ibit); }
  std::vector<bool> getGPI_all() { return getGPI(); }
  void setGPO_single(int ibit, bool t) { return setGPO(ibit,t); }
  void setGPO_all(const std::vector<bool>& b) { return setGPO(b); }

 private:
  /** 
   * Cached numbers of GPI and GPO bits
   */
  int ngpi_, ngpo_;

};

}

#endif // PFLIB_GPIO_H_
