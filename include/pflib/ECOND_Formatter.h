#ifndef pflib_ECOND_Formatter_h_included
#define pflib_ECOND_Formatter_h_included

#include <stdint.h>
#include <vector>

namespace pflib {

  class ECOND_Formatter {
  public:
    ECOND_Formatter(int subsystem_id, int contributor_id);

    void startEvent(int bx, int l1a, int orbit);
    void finishEvent();
    const std::vector<uint32_t>& getPacket() const { return packet_; }
    void disable_zs(bool disable=true) { disable_ZS_=disable; }
    void add_elink_packet(int ielink, const std::vector<uint32_t>& src);
    
  private:
    std::vector<uint32_t> format_elink(int ielink, const std::vector<uint32_t>& src);
    int zs_process(int ielink, int ic, uint32_t word);

    std::vector<uint32_t> packet_;
    
    int burn_count_;
    int subsystem_id_;
    int contributor_id_;
    int sentinel_;

    bool disable_ZS_;
  };
  
}

#endif // pflib_ECOND_Formatter_h_included
