#include <stdint.h>
#include <vector>
#include <iostream>

  // register map : https://gitlab.cern.ch/cms-hgcal-firmware/components/hgcal_slow_control/-/blob/Vivado2019.2/Documentation/SlowControlDocumentation.pdf
#define ERROR_FLAGS_OFFSET 125
#define ERROR_FLAGS_MASK 0x7

#define CHIP_ADDR_OFFSET 113
#define CHIP_ADDR_MASK 0x7F

#define READ_WRITE_OFFSET 112
#define READ_WRITE_MASK 0x1

#define LPGBT_COMMAND_OFFSET 105
#define LPGBT_COMMAND_MASK 0x7F

#define PARITY_FLAG_OFFSET 104
#define PARITY_FLAG_MASK 0x1

#define NREG_OFFSET 88
#define NREG_MASK 0xF

#define REGISTER_ADDR_OFFSET 72
#define REGISTER_ADDR_MASK 0xFFFF

#define PARITY_WORD_OFFSET 64
#define PARITY_WORD_MASK 0xFF

#define PAYLOAD_OFFSET 0
#define PAYLOAD_MASK 0xFFFFFFFFFFFFFFFF

#define ERROR_FLAGS_REL_OFFSET   ERROR_FLAGS_OFFSET % 32
#define CHIP_ADDR_REL_OFFSET     CHIP_ADDR_OFFSET % 32
#define READ_WRITE_REL_OFFSET    READ_WRITE_OFFSET % 32
#define LPGBT_COMMAND_REL_OFFSET LPGBT_COMMAND_OFFSET % 32
#define PARITY_FLAG_REL_OFFSET   PARITY_FLAG_OFFSET % 32
#define NREG_REL_OFFSET          NREG_OFFSET % 32
#define REGISTER_ADDR_REL_OFFSET REGISTER_ADDR_OFFSET % 32
#define PARITY_WORD_REL_OFFSET   PARITY_WORD_OFFSET % 32

struct ic_rx_frame
{
  ic_rx_frame() :
    error_flag(0)   ,
    chip_address(0) ,
    read_write(0)   ,
    lpgbt_command(0),
    parity_flag(0)  ,
    n_reg(0)        ,
    reg_address(0)  ,
    parity_word(0)  ,
    payload(0)      {;}
  
  ic_rx_frame( unsigned int _error_flag, unsigned int _chip_address, unsigned int _read_write, unsigned int _lpgbt_command,
	       unsigned int _parity_flag, unsigned int _n_reg, unsigned int _reg_address, unsigned int _parity_word, uint64_t _payload ) :
    error_flag(_error_flag)      ,
    chip_address(_chip_address)  ,
    read_write(_read_write)      ,
    lpgbt_command(_lpgbt_command),
    parity_flag(_parity_flag)   ,
    n_reg(_n_reg)                ,
    reg_address(_reg_address)    ,
    parity_word(_parity_word)    ,
    payload(_payload)
  { ; }
  friend std::ostream& operator<<(std::ostream& out,const ic_rx_frame& frame)
  {
    out << "ic_tx_frame: (error_flag, chip_address, read_write, lpgbt_command, parity_flag, n_reg, reg_address, parity_word, payload) = "
	<< frame.error_flag    << ", "
	<< frame.chip_address  << ", "
	<< frame.read_write    << ", "
	<< frame.lpgbt_command << ", "
	<< frame.parity_flag   << ", "
	<< frame.n_reg         << ", "
	<< frame.reg_address   << ", "
	<< frame.parity_word   << ", "
	<< frame.payload       ;
    return out;    
  }
  unsigned int error_flag    ;
  unsigned int chip_address  ;
  unsigned int read_write    ;
  unsigned int lpgbt_command ;
  unsigned int parity_flag   ;
  unsigned int n_reg         ;
  unsigned int reg_address   ;
  unsigned int parity_word   ;
  uint64_t     payload       ;
};

class sct_rx_decoder
{
public:
  sct_rx_decoder(){;}
  ~sct_rx_decoder(){;}
  static const ic_rx_frame decode( const std::vector<uint32_t>& encoded_data );
protected:
};
