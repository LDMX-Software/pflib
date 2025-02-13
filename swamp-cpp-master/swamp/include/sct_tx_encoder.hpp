#include <stdint.h>
#include <vector>
#include <iostream>

// register map : https://gitlab.cern.ch/cms-hgcal-firmware/components/hgcal_slow_control/-/blob/Vivado2019.2/Documentation/SlowControlDocumentation.pdf
#define BROADCAST_ADDR_OFFSET 112
#define BROADCAST_ADDR_MASK 0xFFFF

#define REPLY_ADDR_OFFSET 96
#define REPLY_ADDR_MASK 0xF

#define CHIP_ADDR_OFFSET 89
#define CHIP_ADDR_MASK 0x7F

#define READ_WRITE_OFFSET 88
#define READ_WRITE_MASK 0x1

#define REGISTER_ADDR_OFFSET 72
#define REGISTER_ADDR_MASK 0xFFFF

#define NREG_OFFSET 64
#define NREG_MASK 0xF

#define PAYLOAD_OFFSET 0
#define PAYLOAD_MASK 0xFFFFFFFFFFFFFFFF

#define BROADCAST_ADDR_REL_OFFSET BROADCAST_ADDR_OFFSET % 32
#define REPLY_ADDR_REL_OFFSET     REPLY_ADDR_OFFSET     % 32
#define CHIP_ADDR_REL_OFFSET      CHIP_ADDR_OFFSET      % 32
#define READ_WRITE_REL_OFFSET     READ_WRITE_OFFSET     % 32
#define REGISTER_ADDR_REL_OFFSET  REGISTER_ADDR_OFFSET  % 32
#define NREG_REL_OFFSET           NREG_OFFSET           % 32

struct ic_tx_frame
{
  ic_tx_frame() :
    broadcast_address(0),
    reply_address(0),
    chip_address(0),
    read_write(0),
    reg_address(0),
    n_reg(0),
    payload(0)
  {;}
  
  ic_tx_frame(  unsigned int _broadcast_address,  unsigned int _reply_address, unsigned int _chip_address,
	     unsigned int _read_write, unsigned int _reg_address, unsigned int _n_reg, uint64_t _payload ) :
    broadcast_address(_broadcast_address),
    reply_address(_reply_address),
    chip_address(_chip_address),
    read_write(_read_write),
    reg_address(_reg_address),
    n_reg(_n_reg),
    payload(_payload)
  { ; }
  friend std::ostream& operator<<(std::ostream& out,const ic_tx_frame& frame)
  {
    out << "ic_tx_frame: (broadcast_address, reply_address, chip_address, read_write, reg_address, n_reg, payload) = "
	<< frame.broadcast_address << ", "
	<< frame.reply_address << ", "
	<< frame.chip_address << ", "
	<< frame.read_write << ", "
	<< frame.reg_address << ", "
	<< frame.n_reg << ", "
	<< frame.payload ;
    return out;    
  }
  unsigned int broadcast_address;
  unsigned int reply_address;
  unsigned int chip_address;
  unsigned int read_write;
  unsigned int reg_address;
  unsigned int n_reg;
  uint64_t payload;
};

class sct_tx_encoder
{
public:
  sct_tx_encoder(){;}
  ~sct_tx_encoder(){;}
  static const std::vector<uint32_t> encode(const ic_tx_frame& frame);
  static const ic_tx_frame decode( const std::vector<uint32_t>& encoded_data );
protected:
};
