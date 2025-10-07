#ifndef ZCU_OPTOLINK_INCLUDED
#define ZCU_OPTOLINK_INCLUDED 1

#include <map>
#include <vector>

#include "pflib/zcu/UIO.h"

namespace pflib {
namespace zcu {

class OptoLink {
 public:
  OptoLink(const char* coder_name = "singleLPGBT");
  OptoLink(const std::string& name) : OptoLink{name.c_str()} {}

  void reset_link();

  bool get_polarity(int ichan, bool is_rx = true);
  void set_polarity(bool polarity, int ichan, bool is_rx = true);

  std::map<std::string, uint32_t> opto_status();
  std::map<std::string, uint32_t> opto_rates();

  ::pflib::UIO& coder() { return coder_; }

  // setup various aspects
  /// there are four TX elinks configured in the coder block
  int get_elink_tx_mode(int ilink);
  void set_elink_tx_mode(int ilink, int mode);

  void capture_ec(int mode, std::vector<uint8_t>& tx, std::vector<uint8_t>& rx);
  void capture_ic(int mode, std::vector<uint8_t>& tx, std::vector<uint8_t>& rx);

 private:
  ::pflib::UIO transright_;
  ::pflib::UIO coder_;
  std::string coder_name_;
};

}  // namespace zcu
}  // namespace pflib

#endif  // ZCU_OPTOLINK_INCLUDED
