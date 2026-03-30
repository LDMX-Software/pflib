#pragma once
#ifndef pflib_ECOND_Formatter_h_included
#define pflib_ECOND_Formatter_h_included

#include <stdint.h>

#include <vector>

namespace pflib::packing {

/**
 * Helper class emulating the ECON-D's behavior for constructing
 * an event packet
 */
class ECONDFormatter {
 public:
  ECONDFormatter(bool disable_zs);
  void disableZS(bool disable = true);
  void startEvent(int bx, int l1a, int orbit);
  void finishEvent();
  const std::vector<uint32_t>& getPacket() const { return packet_; }
  void add_elink_packet(int ielink, const std::vector<uint32_t>& src);

 private:
  std::vector<uint32_t> format_elink(int ielink,
                                     const std::vector<uint32_t>& src);
  int zs_process(int ielink, int ic, uint32_t word);

  /// packet currently under construction
  std::vector<uint32_t> packet_{};
  /// whether zero-suppresion should be disabled or not
  bool disable_zs_{true};
};

}  // namespace pflib::packing

#endif  // pflib_ECOND_Formatter_h_included
