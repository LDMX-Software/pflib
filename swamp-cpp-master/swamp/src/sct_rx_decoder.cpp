#include "sct_rx_decoder.hpp"
#include <assert.h>

const ic_rx_frame sct_rx_decoder::decode( const std::vector<uint32_t>& encoded_data )
{
  assert( encoded_data.size()==4 );

  ic_rx_frame frame;
  
  frame.error_flag    = ( encoded_data[3] >> ERROR_FLAGS_REL_OFFSET   ) & ERROR_FLAGS_MASK;
  frame.chip_address  = ( encoded_data[3] >> CHIP_ADDR_REL_OFFSET     ) & CHIP_ADDR_MASK;
  frame.read_write    = ( encoded_data[3] >> READ_WRITE_REL_OFFSET    ) & READ_WRITE_MASK;
  frame.lpgbt_command = ( encoded_data[3] >> LPGBT_COMMAND_REL_OFFSET ) & LPGBT_COMMAND_MASK;
  frame.parity_flag   = ( encoded_data[3] >> PARITY_FLAG_REL_OFFSET   ) & PARITY_FLAG_MASK;
  frame.n_reg         = ( encoded_data[2] >> NREG_REL_OFFSET          ) & NREG_MASK;
  frame.reg_address   = ( encoded_data[2] >> REGISTER_ADDR_REL_OFFSET ) & REGISTER_ADDR_MASK;
  frame.parity_word   = ( encoded_data[2] >> PARITY_WORD_REL_OFFSET   ) & PARITY_WORD_MASK;
  frame.payload       = ((uint64_t)encoded_data[1])<<32 | ((uint64_t)encoded_data[0]);
  
  return frame;
}
