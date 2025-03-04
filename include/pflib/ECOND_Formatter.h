#ifndef pflib_ECOND_Formatter_h_included
#define pflib_ECOND_Formatter_h_included

#include <stdint.h>
#include <vector>

namespace pflib {

  class ECOND_Formatter {
  public:
    ECOND_Formatter(int subsystem_id, int conributor_id);
    void test();
    
  private:
    std::vector<uint32_t> format_elink(const std::vector<uint32_t>& src);
    int zs_process(int ic, uint32_t word);
    
    int burn_count_;
    int subsystem_id_;
    int contributor_id_;
    int sentinel_;
  };
  
}

#endif // pflib_ECOND_Formatter_h_included
