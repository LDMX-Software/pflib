#ifndef PFLIB_GPIO_H_
#define PFLIB_GPIO_H_

#include <string>
#include <vector>

namespace pflib {

/**
 * Representation of GPIO controller
 */
class GPIO {
 protected:
  GPIO(int gpo, int gpi) : ngpo_{gpo}, ngpi_{gpi} {}

 public:
  /**
   * Get the number of GPO bits
   */
  int getGPOcount() { return ngpo_; }

  /**
   * Get the number of GPI bits
   */
  int getGPIcount() { return ngpi_; }

  /** Get the name of a bit if possible */
  virtual std::string getBitName(int ibit, bool isgpo = true) { return ""; }

  /**
   * Read a GPI bit
   */
  virtual bool getGPI(int ibit);

  /**
   * Read all GPI bits
   */
  virtual std::vector<bool> getGPI() = 0;

  /**
   * Set a single GPO bit
   */
  virtual void setGPO(int ibit, bool toTrue = true);

  /**
   * Set all GPO bits
   */
  virtual void setGPO(const std::vector<bool>& bits) = 0;

  /**
   * Read all GPO bits
   */
  virtual std::vector<bool> getGPO() = 0;

  /// convenience wrapper for python bindings
  bool getGPI_single(int ibit) { return getGPI(ibit); }
  std::vector<bool> getGPI_all() { return getGPI(); }
  void setGPO_single(int ibit, bool t) { return setGPO(ibit, t); }
  void setGPO_all(const std::vector<bool>& b) { return setGPO(b); }

 private:
  /**
   * Cached numbers of GPI and GPO bits
   */
  int ngpi_, ngpo_;
};

GPIO* make_GPIO_HcalHGCROCZCU();

}  // namespace pflib

#endif  // PFLIB_GPIO_H_
