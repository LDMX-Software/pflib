#include "sct_tx_encoder.hpp"
#include <assert.h>

const std::vector<uint32_t> sct_tx_encoder::encode(const ic_tx_frame& frame)
{
  assert( frame.payload <= PAYLOAD_MASK );
  assert( frame.n_reg <= NREG_MASK );
  assert( frame.reg_address <= REGISTER_ADDR_MASK );
  assert( frame.chip_address <= CHIP_ADDR_MASK );
  assert( frame.read_write <= READ_WRITE_MASK );
  assert( frame.reply_address <= REPLY_ADDR_MASK );
  assert( frame.broadcast_address <= BROADCAST_ADDR_MASK );

  std::vector<uint32_t> encoded_data(4);
  
  encoded_data[3]  = (frame.broadcast_address & BROADCAST_ADDR_MASK) << BROADCAST_ADDR_REL_OFFSET;
  encoded_data[3] |= (frame.reply_address     & REPLY_ADDR_MASK)     << REPLY_ADDR_REL_OFFSET;

  encoded_data[2]  = (frame.chip_address & CHIP_ADDR_MASK) << CHIP_ADDR_REL_OFFSET;
  encoded_data[2] |= (frame.read_write & READ_WRITE_MASK)  << READ_WRITE_REL_OFFSET;
  encoded_data[2] |= (frame.reg_address & REGISTER_ADDR_MASK) << REGISTER_ADDR_REL_OFFSET;
  encoded_data[2] |= (frame.n_reg & NREG_MASK) << NREG_REL_OFFSET;

  encoded_data[1] |= ((frame.payload & PAYLOAD_MASK) >> 32) & 0xFFFFFFFF ;
  encoded_data[0] |= ((frame.payload & PAYLOAD_MASK) >> 0)  & 0xFFFFFFFF ;
  
  return encoded_data;
}

const ic_tx_frame sct_tx_encoder::decode( const std::vector<uint32_t>& encoded_data )
{
  assert( encoded_data.size()==4 );

  ic_tx_frame frame;
  frame.broadcast_address = ( encoded_data[3] >> BROADCAST_ADDR_REL_OFFSET ) & BROADCAST_ADDR_MASK;
  frame.reply_address     = ( encoded_data[3] >> REPLY_ADDR_REL_OFFSET     ) & REPLY_ADDR_MASK;
  frame.chip_address      = ( encoded_data[2] >> CHIP_ADDR_REL_OFFSET      ) & CHIP_ADDR_MASK;
  frame.read_write        = ( encoded_data[2] >> READ_WRITE_REL_OFFSET     ) & READ_WRITE_MASK;
  frame.reg_address       = ( encoded_data[2] >> REGISTER_ADDR_REL_OFFSET  ) & REGISTER_ADDR_MASK;
  frame.n_reg             = ( encoded_data[2] >> NREG_REL_OFFSET           ) & NREG_MASK;
  frame.payload           = ((uint64_t)encoded_data[1])<<32 | ((uint64_t)encoded_data[0]);
  return frame;
}
