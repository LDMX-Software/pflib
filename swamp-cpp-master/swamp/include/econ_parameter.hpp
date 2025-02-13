#pragma once

namespace ECON{

  enum class PARAM_ACCESS {
    READ_WRITE,
    READ_ONLY,
    WRITE_ONLY
  };

  struct ECONParam{
    constexpr ECONParam( ) : 
      address(0),
      access(PARAM_ACCESS::READ_WRITE),
      size_byte(1),
      default_value(0xFF),
      param_mask(0xFF),
      param_shift(0) {}
    constexpr ECONParam( uint16_t aAddress,
			 PARAM_ACCESS aAccess,
			 uint16_t aSize_byte,
			 uint32_t aDefault_value,
			 uint32_t aParam_mask,
			 uint16_t aParam_shift ) : 
      address(aAddress),
      access(aAccess),
      size_byte(aSize_byte),
      default_value(aDefault_value),
      param_mask(aParam_mask),
      param_shift(aParam_shift) {}

    uint16_t address;
    PARAM_ACCESS access;
    uint16_t size_byte;
    uint32_t default_value;
    uint32_t param_mask;
    uint16_t param_shift;
  };

}
