#ifndef ZCU_OPTOLINK_INCLUDED
#define ZCU_OPTOLINK_INCLUDED 1

#include "pflib/zcu/UIO.h"
#include <map>

namespace pflib {
namespace zcu {

class OptoLink {
 public:
  OptoLink();

  void reset_link();

  bool get_polarity(int ichan, bool is_rx=true);
  void set_polarity(bool polarity, int ichan, bool is_rx=true);

  std::map<std::string, uint32_t> opto_status();
  std::map<std::string, uint32_t> opto_rates();
  
 private:
  ::pflib::UIO transright_;
  
};

}
}

#endif // ZCU_OPTOLINK_INCLUDED
