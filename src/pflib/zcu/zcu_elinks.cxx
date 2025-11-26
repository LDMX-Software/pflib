#include "pflib/zcu/zcu_elinks.h"

#include "pflib/utility/string_format.h"

namespace pflib {
namespace zcu {

/** Currently represents all elinks for dual-link configuration */
OptoElinksZCU::OptoElinksZCU(lpGBT* lpdaq, lpGBT* lptrig, int itarget)
    : Elinks(6 * 2),
      lp_daq_(lpdaq),
      lp_trig_(lptrig),
      uiodecoder_(
          pflib::utility::string_format("standardLpGBTpair-%d", itarget)) {
  // deactivate all the links except DAQ link 0 for now
  for (int i = 1; i < 6 * 2; i++) markActive(i, false);
}
std::vector<uint32_t> OptoElinksZCU::spy(int ilink) {
  std::vector<uint32_t> retval;
  // spy now...
  static constexpr int REG_CAPTURE_ENABLE = 16;
  static constexpr int REG_CAPTURE_OLINK = 17;
  static constexpr int REG_CAPTURE_ELINK = 18;
  static constexpr int REG_CAPTURE_PTR = 19;
  static constexpr int REG_CAPTURE_WINDOW = 20;
  uiodecoder_.write(REG_CAPTURE_OLINK, ilink % 6);
  uiodecoder_.write(REG_CAPTURE_ELINK, (ilink / 6 + 1) & 0x7);
  uiodecoder_.write(REG_CAPTURE_ENABLE, 0);
  uiodecoder_.write(REG_CAPTURE_ENABLE, 1);
  usleep(1000);
  uiodecoder_.write(REG_CAPTURE_ENABLE, 0);
  for (int i = 0; i < 64; i++) {
    uiodecoder_.write(REG_CAPTURE_PTR, i);
    usleep(1);
    retval.push_back(uiodecoder_.read(REG_CAPTURE_WINDOW));
  }
  return retval;
}

void OptoElinksZCU::setBitslip(int ilink, int bitslip) {
  static constexpr int REG_UPLINK_PHASE = 21;
  uint32_t val = uiodecoder_.read(REG_UPLINK_PHASE + ilink / 6);
  uint32_t mask = 0x1F << ((ilink % 6) * 5);
  // set to zero
  val = val | mask;
  val = val ^ mask;
  // mask in new phase
  val = val | ((bitslip & 0x1F) << ((ilink % 6) * 5));
  uiodecoder_.write(REG_UPLINK_PHASE + ilink / 6, val);
}
int OptoElinksZCU::getBitslip(int ilink) {
  static constexpr int REG_UPLINK_PHASE = 21;
  uint32_t val = uiodecoder_.read(REG_UPLINK_PHASE + (ilink / 6));
  return (val >> ((ilink % 6) * 5)) & 0x1F;
}

}  // namespace zcu
}  // namespace pflib
