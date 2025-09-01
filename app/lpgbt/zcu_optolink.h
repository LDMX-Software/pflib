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

  ::pflib::UIO& coder() { return coder_; }

  // setup various aspects
  /// there are four TX elinks configured in the coder block
  int get_elink_tx_mode(int ilink);
  void set_elink_tx_mode(int ilink, int mode);
  
 private:
  ::pflib::UIO transright_;
  ::pflib::UIO coder_;
  
};

}
}

#endif // ZCU_OPTOLINK_INCLUDED
