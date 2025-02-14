#pragma once

namespace LPGBT
{

  struct LPGBTReg{
    constexpr LPGBTReg() :
      address(0) {}
    constexpr LPGBTReg(uint16_t aAddress) :
      address(aAddress) {}
    uint16_t address;
  };

}
