#ifndef LPGBT_MEZZ_TESTER_H_INCLUDED
#define LPGBT_MEZZ_TESTER_H_INCLUDED

#include <vector>
#include "pflib/zcu/UIO.h"

class LPGBT_Mezz_Tester {
public:
  LPGBT_Mezz_Tester();

  std::vector<float> clock_rates();
  
private:
  pflib::UIO uio_;
};

#endif // LPGBT_MEZZ_TESTER_H_INCLUDED
