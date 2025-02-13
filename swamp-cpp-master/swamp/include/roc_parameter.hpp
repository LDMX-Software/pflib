#pragma once

namespace HGCROC{

  struct RocReg{
    constexpr RocReg( ) : R0(256),
			  R1(256),
			  defval_mask(0),
			  param_mask(0),
			  reg_mask(0),
			  param_minbit(0) {}
    constexpr RocReg( uint16_t aR0,
		      uint16_t aR1,
		      uint16_t aDefval_mask,
		      uint32_t aParam_mask,
		      uint16_t aReg_mask,
		      uint16_t aParam_minbit ) : R0(aR0),
						 R1(aR1),
						 defval_mask(aDefval_mask),
						 param_mask(aParam_mask),
						 reg_mask(aReg_mask),
						 param_minbit(aParam_minbit) {}
    uint16_t R0;
    uint16_t R1;
    uint16_t defval_mask;
    uint32_t param_mask;
    uint16_t reg_mask;
    uint16_t param_minbit;
  };


  struct DirectAccessReg{
    constexpr DirectAccessReg( ) : address(9),
				   defval_mask(0),
				   param_mask(0),
				   reg_mask(0) {}
    constexpr DirectAccessReg( uint16_t aAddress,
			       uint16_t aDefval_mask,
			       uint32_t aParam_mask,
			       uint16_t aReg_mask ) : address(aAddress),
				                      defval_mask(aDefval_mask),
				                      param_mask(aParam_mask),
						      reg_mask(aReg_mask) {}
    uint16_t address;
    uint16_t defval_mask;
    uint32_t param_mask;
    uint16_t reg_mask;
  };
  
}
