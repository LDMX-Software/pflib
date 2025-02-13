#pragma once

namespace VTRXP{

  struct VTRXpParam{
    constexpr VTRXpParam( ) :
      bit_mask(0x0),
      offset(0x0),
      default_value(0x0),
      address(0) {}
    constexpr VTRXpParam( uint32_t a_bit_mask,
			  uint16_t a_offset,
			  uint32_t a_default_value,
			  uint8_t  a_address ) : bit_mask(a_bit_mask),
						 offset(a_offset),
						 default_value(a_default_value),
						 address(a_address) {}
    uint32_t bit_mask;
    uint16_t offset;
    uint32_t default_value;
    uint8_t address;

  };

}
