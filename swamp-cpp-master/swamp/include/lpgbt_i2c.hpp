#pragma once

#include "transport.hpp"
#include "redis-monitoring.hpp"

#include <sstream>

#define NUMBER_OF_I2C_MASTER 3
#define MAXIMUM_NUMBER_OF_I2C_ATTEMPTS 100

#define I2CM0CMD_ADDRESS 0x0106
#define I2CM0STATUS_ADDRESS 0x0171
#define I2CM0ADDRESS_ADDRESS 0x0101
#define I2CM0CTRL_ADDRESS 0x016f
#define I2CM0DATA0_ADDRESS 0x0102
#define I2CM0READBYTE_ADDRESS 0x0173
#define I2CM0READ15_ADDRESS 0x0183
#define I2CM1CMD_ADDRESS 0x010D
#define I2CM1STATUS_ADDRESS 0x0186

struct I2CM0CONFIG{
  constexpr static uint16_t ADDRESS = 0x0100;
  
  constexpr static uint16_t I2CM0ADDRESSEXT_BIT_MASK = 7;
  constexpr static uint16_t I2CM0ADDRESSEXT_LENGTH = 3;
  constexpr static uint16_t I2CM0ADDRESSEXT_OFFSET = 0;
  
  constexpr static uint16_t I2CM0SDADRIVESTRENGTH_BIT_MASK = 8;
  constexpr static uint16_t I2CM0SDADRIVESTRENGTH_LENGTH = 1;
  constexpr static uint16_t I2CM0SDADRIVESTRENGTH_OFFSET = 3;
  
  constexpr static uint16_t I2CM0SDAPULLUPENABLE_BIT_MASK = 16;
  constexpr static uint16_t I2CM0SDAPULLUPENABLE_LENGTH = 1;
  constexpr static uint16_t I2CM0SDAPULLUPENABLE_OFFSET = 4;
  
  constexpr static uint16_t I2CM0SCLDRIVESTRENGTH_BIT_MASK = 32;
  constexpr static uint16_t I2CM0SCLDRIVESTRENGTH_LENGTH = 1;
  constexpr static uint16_t I2CM0SCLDRIVESTRENGTH_OFFSET = 5;

  constexpr static uint16_t I2CM0SCLPULLUPENABLE_BIT_MASK = 64;
  constexpr static uint16_t I2CM0SCLPULLUPENABLE_LENGTH = 1;
  constexpr static uint16_t I2CM0SCLPULLUPENABLE_OFFSET = 6;
};


struct RST0{
  constexpr static uint16_t ADDRESS = 0x013c;

  constexpr static uint16_t RSTCONFIG_BIT_MASK = 32;
  constexpr static uint16_t RSTCONFIG_LENGTH = 1;
  constexpr static uint16_t RSTCONFIG_OFFSET = 5;

  constexpr static uint16_t RSTFUSES_BIT_MASK = 64;
  constexpr static uint16_t RSTFUSES_LENGTH = 1;
  constexpr static uint16_t RSTFUSES_OFFSET = 6;

  constexpr static uint16_t RSTI2CM0_BIT_MASK = 4;
  constexpr static uint16_t RSTI2CM0_LENGTH = 1;
  constexpr static uint16_t RSTI2CM0_OFFSET = 2;

  constexpr static uint16_t RSTI2CM1_BIT_MASK = 2;
  constexpr static uint16_t RSTI2CM1_LENGTH = 1;
  constexpr static uint16_t RSTI2CM1_OFFSET = 1;

  constexpr static uint16_t RSTI2CM2_BIT_MASK = 1;
  constexpr static uint16_t RSTI2CM2_LENGTH = 1;
  constexpr static uint16_t RSTI2CM2_OFFSET = 0;

  constexpr static uint16_t RSTPLLDIGITAL_BIT_MASK = 128;
  constexpr static uint16_t RSTPLLDIGITAL_LENGTH = 1;
  constexpr static uint16_t RSTPLLDIGITAL_OFFSET = 7;

  constexpr static uint16_t RSTRXLOGIC_BIT_MASK = 16;
  constexpr static uint16_t RSTRXLOGIC_LENGTH = 1;
  constexpr static uint16_t RSTRXLOGIC_OFFSET = 4;

  constexpr static uint16_t RSTTXLOGIC_BIT_MASK = 8;
  constexpr static uint16_t RSTTXLOGIC_LENGTH = 1;
  constexpr static uint16_t RSTTXLOGIC_OFFSET = 3;
};

enum I2cmStatusReg{
		   SUCCESS  =0X04,
		   LEVEERR  =0x08,
		   NOACK    =0x40,
		   TIMEOUT  =0xFF
};

enum I2cmCommand{
		 WRITE_CRA = 0x0,
		 WRITE_MSK = 0x1,
		 ONE_BYTE_WRITE = 0x2,
		 ONE_BYTE_READ = 0x3,
		 ONE_BYTE_WRITE_EXT = 0x4,
		 ONE_BYTE_READ_EXT = 0x5,
		 ONE_BYTE_RMW_OR = 0x6,
		 ONE_BYTE_RMW_XOR = 0x7,
		 W_MULTI_4BYTE0 = 0x8,
		 W_MULTI_4BYTE1 = 0x9,
		 W_MULTI_4BYTE2 = 0xA,
		 W_MULTI_4BYTE3 = 0xB,
		 WRITE_MULTI = 0xC,
		 READ_MULTI = 0xD,
		 WRITE_MULTI_EXT = 0xE,
		 READ_MULTI_EXT = 0xF
};

constexpr std::array<int,4> commandToExecute{
  I2cmCommand::ONE_BYTE_WRITE,
  I2cmCommand::ONE_BYTE_READ,
  I2cmCommand::WRITE_MULTI,
  I2cmCommand::READ_MULTI
};

struct I2cmConfigReg{
  constexpr static uint16_t FREQ_OFFSET = 0;
  constexpr static uint16_t FREQ_LENGTH = 2;
  constexpr static uint16_t FREQ_BIT_MASK = 3;

  constexpr static uint16_t NBYTES_OFFSET = 2;
  constexpr static uint16_t NBYTES_LENGTH = 5;
  constexpr static uint16_t NBYTES_BIT_MASK = 0x7C;

  constexpr static uint16_t SCLDRIVE_OFFSET = 7;
  constexpr static uint16_t SCLDRIVE_LENGTH = 1;
  constexpr static uint16_t SCLDRIVE_BIT_MASK = 0x80;
};

struct I2CTransactionStatus
{
  I2CTransactionStatus(){;}
  I2CTransactionStatus(I2cmStatusReg aStatus) : status(aStatus)
  {;}
  I2cmStatusReg status;
};

struct TransactionBuilder{
  TransactionBuilder(){}
  void clear(){
    regs.clear();
    vals.clear();
  }
  void addTransaction(unsigned int reg, std::vector<unsigned int> val_vec){
    regs.push_back(reg);
    vals.push_back(val_vec);
  }
  std::vector<unsigned int> regs;
  std::vector< std::vector<unsigned int> > vals;
};

class LPGBT_I2C : public Transport
{
public:

  LPGBT_I2C( const std::string& name, const YAML::Node& config );

  ~LPGBT_I2C()
  {
  }

  void write_regs(const unsigned int addr, const unsigned int reg_addr_width, const unsigned int reg_addr, const std::vector<unsigned int>& reg_vals);
    
  void write_reg(const unsigned int addr, const unsigned int reg_val);
  
  unsigned int read_reg(const unsigned int addr);
  
  std::vector<unsigned int> read_regs(const unsigned int addr, const unsigned int reg_addr_width, const unsigned int reg_addr, const unsigned int read_len);

  std::vector<unsigned int> read_regs(const unsigned int addr, const unsigned int read_len);
  
protected:

  void init_monitoring();
  void master_set_slave_address(unsigned int slave_address);
  void master_issue_command(unsigned int command,bool executeNow=false);
  void execute_transactions(unsigned int command);
  void master_set_nbytes(unsigned int nbytes);
  I2CTransactionStatus master_await_completion();
  
  void ResetImpl() override;

  void ConfigureImpl(const YAML::Node& node) override;

private:
  unsigned int m_offset_write;
  unsigned int m_offset_read;
  unsigned int m_master_id;
  bool m_addr_10bit;
  unsigned int m_timeout;  
  unsigned int MAX_MULTI_BYTES_WRITE;
  unsigned int MAX_MULTI_BYTES_READ;

  RedisMonitoring mTransactionMonitor;
  TransactionBuilder mTransactionBuilder;
};
