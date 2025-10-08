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
  GPIO() {}

 public:
  /**
   * Get the set of GPO pin names
   */
  virtual std::vector<std::string> getGPOs() = 0;

  /**
   * Get the set of GPI pin names
   */
  virtual std::vector<std::string> getGPIs() = 0;

  /** Check if a given GPO exists */
  virtual bool hasGPO(const std::string& name);

  /** Check if a given GPI exists */
  virtual bool hasGPI(const std::string& name);

  /**
   * Read a GPI bit
   */
  virtual bool getGPI(const std::string& name) = 0;

  /**
   * Get current value of GPO bit
   */
  virtual bool getGPO(const std::string& name) = 0;

  /**
   * Set a single GPO bit
   */
  virtual void setGPO(const std::string& name, bool toTrue = true) = 0;
};

GPIO* make_GPIO_HcalHGCROCZCU();

}  // namespace pflib

#endif  // PFLIB_GPIO_H_
