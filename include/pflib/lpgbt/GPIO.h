#ifndef pflib_lpgbt_GPIO_h_included
#define pflib_lpgbt_GPIO_h_included

#include <map>

#include "pflib/GPIO.h"

namespace pflib {

class lpGBT;

namespace lpgbt {

class GPIO : public ::pflib::GPIO {
 public:
  GPIO(lpGBT& lpgbt) : lpgbt_(lpgbt) {}
  virtual std::vector<std::string> getGPOs();
  virtual std::vector<std::string> getGPIs();
  virtual bool getGPI(const std::string& name);
  virtual bool getGPO(const std::string& name);
  virtual void setGPO(const std::string& name, bool toTrue = true);

 protected:
  friend class ::pflib::lpGBT;
  void add_pin(const std::string& name, int ibit, bool output);

 private:
  lpGBT& lpgbt_;
  std::map<std::string, int> gpos_, gpis_;
};

}  // namespace lpgbt
}  // namespace pflib

#endif  // pflib_lpgbt_GPIO_h_included
