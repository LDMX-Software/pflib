#ifndef LPGBT_MEZZ_TESTER_H_INCLUDED
#define LPGBT_MEZZ_TESTER_H_INCLUDED

#include <vector>

#include "pflib/zcu/UIO.h"

class LPGBT_Mezz_Tester {
 public:
  LPGBT_Mezz_Tester(pflib::UIO& opto);
  ~LPGBT_Mezz_Tester();
  std::vector<float> clock_rates();

  void get_mode(bool& addr, bool& mode1);
  void set_mode(bool addr, bool mode1);
  void reset_lpGBT();

  void set_prbs_len_ms(int len);
  void set_phase(int phase, int ilink = -1);
  int get_phase(int ilink = -1);
  void set_uplink_pattern(int ilink, int pattern);
  std::vector<uint32_t> ber_rx();
  std::vector<uint32_t> ber_tx();

  std::vector<uint32_t> capture(int ilink, bool is_rx = false);

 private:
  pflib::UIO& opto_;
  pflib::UIO* wired_;
};

#endif  // LPGBT_MEZZ_TESTER_H_INCLUDED
