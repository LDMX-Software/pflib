#include "lpgbt_i2c.hpp"
#include <boost/timer/timer.hpp>

LPGBT_I2C::LPGBT_I2C( const std::string& name, const YAML::Node& config ) : Transport(name,config)
{
  if( config["bus"].IsDefined() )
      m_master_id = config["bus"].as<unsigned int>();
  else{
    auto errorMsg = "'bus' key is missing in {} transport configuration";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name); });
    throw swamp::LPGBTI2CDefinitionErrorException(errorMsg);
  }
  if( m_master_id>=NUMBER_OF_I2C_MASTER ){
    auto errorMsg = "'bus' configuration of {} must be lower than {}";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name,NUMBER_OF_I2C_MASTER); });
    throw swamp::LPGBTI2CDefinitionErrorException(errorMsg);
  }
  m_offset_write = m_master_id*(I2CM1CMD_ADDRESS-I2CM0CMD_ADDRESS);
  m_offset_read = m_master_id*(I2CM1STATUS_ADDRESS-I2CM0STATUS_ADDRESS);
  // to transform into configurable params
  m_addr_10bit=false;
  m_timeout=1;
  MAX_MULTI_BYTES_WRITE = m_addr_10bit ? 13 : 14;
  MAX_MULTI_BYTES_READ  = m_addr_10bit ? 14 : 15;
  init_monitoring();
  mTransactionBuilder.clear();
}

void LPGBT_I2C::init_monitoring()
{
  std::unordered_map<std::string,uint32_t> counterMap{ {"nTransaction"        ,0},
						       {"nSuccess"            ,0},
						       {"nMultiByteW"         ,0},
						       {"nMultiByteWSuccess"  ,0},
						       {"nMultiByteR"         ,0},
						       {"nMultiByteRSuccess"  ,0},
						       {"nSingleByteW"        ,0},
						       {"nSingleByteWSuccess" ,0},
						       {"nSingleByteR"        ,0},
						       {"nSingleByteRSuccess" ,0},
						       {"nNack"               ,0},
						       {"nTimeout"            ,0},
						       {"nI2CFailure"         ,0},
						       {"nSDALow"             ,0}};
  mTransactionMonitor = RedisMonitoring(m_name,counterMap);
}

void LPGBT_I2C::write_regs(const unsigned int addr, const unsigned int reg_addr_width, const unsigned int reg_addr, const std::vector<unsigned int>& reg_vals)
{
  unsigned int transaction_index=0;
  std::vector<unsigned int> data;
  data.reserve(reg_addr_width+reg_vals.size());
  while(1){
    auto start = reg_vals.cbegin() + transaction_index*MAX_MULTI_BYTES_WRITE;
    auto last = (1+transaction_index)*MAX_MULTI_BYTES_WRITE>reg_vals.size() ? reg_vals.cend() : start + MAX_MULTI_BYTES_WRITE;
    std::vector<unsigned int> tmpvec( start, last );
    data.clear();
    unsigned reg_address = reg_addr + transaction_index*MAX_MULTI_BYTES_WRITE;
    for( unsigned int i=reg_addr_width; i>0; i--){\
      auto addr = (reg_address >> (8 * (i-1))) & 0xFF;
      data.push_back( addr );
    }
    std::copy( tmpvec.cbegin(), tmpvec.cend(), std::back_inserter(data) );

    // these condition should never happen
    if( m_addr_10bit && data.size()>15 ){
      auto errorMsg = "Problem in {}, maximum write length is 15 bytes in 10 bit addressing mode";
      spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name); });
      throw swamp::LPGBTI2CUnlikelyException(errorMsg);
    }
    else if( data.size()>16 ){
      auto errorMsg = "Problem in {}, maximum write length is 16 bytes in 7 bit addressing mode";
      spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name); });
      throw swamp::LPGBTI2CUnlikelyException(errorMsg);
    }
    //
    master_set_nbytes(data.size());

    //we fill I2C data buffer 4 bytes at the time, for up to 15/16 bytes in total
    unsigned int i4bytes=0;
    while(1){
      assert( i4bytes<4 );
      auto start4b = data.cbegin() + i4bytes * 4;
      auto last4b = (1+i4bytes)*4>data.size() ? data.cend() : start4b + 4;
      auto vec = std::vector<unsigned int>(start4b,last4b);
      mTransactionBuilder.addTransaction( I2CM0DATA0_ADDRESS + m_offset_write, vec );
      master_issue_command(I2cmCommand::W_MULTI_4BYTE0 + i4bytes);
      if( last4b == data.cend() )
      	break;
      i4bytes++;
    }
    //
    master_set_slave_address(addr);
    mTransactionMonitor.incrementCounter("nMultiByteW");
    execute_transactions(I2cmCommand::WRITE_MULTI);
    mTransactionMonitor.incrementCounter("nMultiByteWSuccess");
    if( last == reg_vals.cend() )
      break;
    transaction_index++;
  }
}

void LPGBT_I2C::write_reg(const unsigned int addr, const unsigned int reg_val)
{
  /*
    Performs a single byte write using an lpGBT I2C masters
  */
  mTransactionBuilder.addTransaction( I2CM0DATA0_ADDRESS + m_offset_write, std::vector<unsigned int>{reg_val} );
  master_set_slave_address(addr);
  mTransactionMonitor.incrementCounter("nSingleByteW");
  execute_transactions(I2cmCommand::ONE_BYTE_WRITE);
  mTransactionMonitor.incrementCounter("nSingleByteWSuccess");
}

std::vector<unsigned int> LPGBT_I2C::read_regs(const unsigned int addr, const unsigned int reg_addr_width, const unsigned int reg_addr, const unsigned int read_len)
{
  unsigned int transaction_index=0;
  master_set_slave_address(addr);
  unsigned int bytes_to_read = read_len;
  std::vector<unsigned int> ret_vec; ret_vec.reserve(read_len); 
  while(bytes_to_read){
    std::vector<unsigned int> vec;vec.reserve(2);//often 16bits address
    auto reg_address = reg_addr + transaction_index * MAX_MULTI_BYTES_READ;
    for( unsigned int i=reg_addr_width; i>0; i--){\
      auto addr = (reg_address >> (8 * (i-1))) & 0xFF;
      vec.push_back( addr );
    }
    mTransactionBuilder.addTransaction( I2CM0DATA0_ADDRESS + m_offset_write, vec );
    mTransactionMonitor.incrementCounter("nMultiByteW");
    master_issue_command(I2cmCommand::W_MULTI_4BYTE0);
    master_set_nbytes(reg_addr_width);
    execute_transactions(I2cmCommand::WRITE_MULTI);
    mTransactionMonitor.incrementCounter("nMultiByteWSuccess");

    auto nbytes = bytes_to_read>MAX_MULTI_BYTES_READ ? MAX_MULTI_BYTES_READ : bytes_to_read;
    mTransactionMonitor.incrementCounter("nMultiByteR");
    master_set_nbytes(nbytes);
    execute_transactions(I2cmCommand::READ_MULTI);
    mTransactionMonitor.incrementCounter("nMultiByteRSuccess");

    unsigned int first_addr = I2CM0READ15_ADDRESS + m_offset_read - (nbytes-1);
    std::vector<unsigned int> read_values = m_carrier->read_regs(first_addr, nbytes);
    std::copy( read_values.rbegin(), read_values.rend(), std::back_inserter(ret_vec) );
    bytes_to_read-=nbytes;
    transaction_index++;
  }
  std::ostringstream os(std::ostringstream::ate);
  os.str("");
  std::for_each( ret_vec.begin(), ret_vec.end(), [&os](auto& v){os << v << " ";} );
  spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : reading addr {}, reg_addr {}, read_len {}, values {}",m_name, addr, reg_addr, read_len, os.str().c_str()); });
  return ret_vec;

}

std::vector<unsigned int> LPGBT_I2C::read_regs(const unsigned int addr, const unsigned int read_len)
{
  master_set_slave_address(addr);
  unsigned int bytes_to_read = read_len;
  std::vector<unsigned int> ret_vec; ret_vec.reserve(read_len); 
  while(bytes_to_read){
    auto nbytes = bytes_to_read>MAX_MULTI_BYTES_READ ? MAX_MULTI_BYTES_READ : bytes_to_read;
    master_set_nbytes(nbytes);
    mTransactionMonitor.incrementCounter("nMultiByteR");
    execute_transactions(I2cmCommand::READ_MULTI);
    mTransactionMonitor.incrementCounter("nMultiByteRSuccess");

    unsigned int first_addr = I2CM0READ15_ADDRESS + m_offset_read - (nbytes-1);
    std::vector<unsigned int> read_values = m_carrier->read_regs(first_addr, nbytes);
    std::copy( read_values.cbegin(), read_values.cend(), std::back_inserter(ret_vec) );
    bytes_to_read-=nbytes;
  }
  return ret_vec;
}

unsigned int LPGBT_I2C::read_reg(const unsigned int addr)
{
  master_set_slave_address(addr);
  mTransactionMonitor.incrementCounter("nSingleByteR");
  execute_transactions(I2cmCommand::ONE_BYTE_READ);
  mTransactionMonitor.incrementCounter("nSingleByteRSuccess");

  unsigned int read_value = m_carrier->read_reg(I2CM0READBYTE_ADDRESS + m_offset_read);
  return read_value;
}

void LPGBT_I2C::ResetImpl()
{
  unsigned int val;
  if( m_master_id==0 )
    val = RST0::RSTI2CM0_BIT_MASK;
  else if( m_master_id==1 )
    val = RST0::RSTI2CM1_BIT_MASK;
  else
    val = RST0::RSTI2CM2_BIT_MASK;
  m_carrier->write_reg(RST0::ADDRESS, val);
  m_carrier->write_reg(RST0::ADDRESS, 0x0);
}

void LPGBT_I2C::ConfigureImpl(const YAML::Node& node)
{
  if( !node["clk_freq"].IsDefined() ||
      !node["scl_drive"].IsDefined() ||
      !node["scl_pullup"].IsDefined() ||
      !node["scl_drive_strength"].IsDefined() ||
      !node["sda_pullup"].IsDefined() ||
      !node["sda_drive_strength"].IsDefined() ){
    auto errorMsg = "Wrong YAML configuration provided to {}. It must contain 'clk_freq', 'scl_drive', 'scl_pullup', 'scl_drive_strength', 'sda_pullup' and 'sda_drive_strength' keys";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name); });
    throw swamp::LPGBTI2CConfigurationErrorException(errorMsg);
  }
  auto clk_freq           = node["clk_freq"].as<unsigned int>();
  auto scl_drive          = node["scl_drive"].as<unsigned int>();
  auto scl_pullup         = node["scl_pullup"].as<unsigned int>();
  auto scl_drive_strength = node["scl_drive_strength"].as<unsigned int>();
  auto sda_pullup         = node["sda_pullup"].as<unsigned int>();
  auto sda_drive_strength = node["sda_drive_strength"].as<unsigned int>();
  if( clk_freq>3 ){
    auto errorMsg = "Problem in configure of {}, clk_freq must not be greater that 3";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name); });
    throw swamp::LPGBTI2CConfigurationErrorException(errorMsg);
  }
  if( scl_drive>1 ){
    auto errorMsg = "Problem in configure of {}, scl_drive must not be greater that 1";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name); });
    throw swamp::LPGBTI2CConfigurationErrorException(errorMsg);
  }
  if( scl_pullup>1 ){
    auto errorMsg = "Problem in configure of {}, scl_pullup must not be greater that 1";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name); });
    throw swamp::LPGBTI2CConfigurationErrorException(errorMsg);
  }
  if( scl_drive_strength>1 ){
    auto errorMsg = "Problem in configure of {}, scl_drive_strength must not be greater that 1";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name); });
    throw swamp::LPGBTI2CConfigurationErrorException(errorMsg);
  }
  if( sda_pullup>1 ){
    auto errorMsg = "Problem in configure of {}, sda_pullup must not be greater that 1";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name); });
    throw swamp::LPGBTI2CConfigurationErrorException(errorMsg);
  }
  if( sda_drive_strength>1 ){
    auto errorMsg = "Problem in configure of {}, sda_drive_strength must not be greater that 1";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name); });
    throw swamp::LPGBTI2CConfigurationErrorException(errorMsg);
  }

  m_carrier->write_reg( I2CM0DATA0_ADDRESS + m_offset_write,
			clk_freq << I2cmConfigReg::FREQ_OFFSET | scl_drive << I2cmConfigReg::SCLDRIVE_OFFSET );
  
  master_issue_command(I2cmCommand::WRITE_CRA,true);
  
  m_carrier->write_reg(I2CM0CONFIG::ADDRESS + m_offset_write,
		       scl_pullup << I2CM0CONFIG::I2CM0SCLPULLUPENABLE_OFFSET |
		       scl_drive_strength << I2CM0CONFIG::I2CM0SCLDRIVESTRENGTH_OFFSET |
		       sda_pullup << I2CM0CONFIG::I2CM0SDAPULLUPENABLE_OFFSET |
		       sda_drive_strength << I2CM0CONFIG::I2CM0SDADRIVESTRENGTH_OFFSET );
}


void LPGBT_I2C::master_set_slave_address( unsigned int slave_address )
{
  if( m_addr_10bit ){
    if( slave_address>1024 ){
      auto errorMsg = "Providing I2C slave address ({}) larger than 1024 is forbidden in 10bit addressing mode";
      spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,slave_address); });
      throw swamp::LPGBTI2CSlaveAddressErrorException(errorMsg);
    }
  }
  else{
    if( slave_address>128 ){
      auto errorMsg = "Providing I2C slave address ({}) larger than 128 is forbidden in 7bit addressing mode";
      spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,slave_address); });
      throw swamp::LPGBTI2CSlaveAddressErrorException(errorMsg);
    }
  }
  
  auto address_low = slave_address & 0x7F;

  mTransactionBuilder.addTransaction( I2CM0ADDRESS_ADDRESS + m_offset_write, std::vector<unsigned int>{address_low} );

  if( m_addr_10bit ){
    auto address_high = (slave_address >> 7) & 0x07;
    auto config_reg_val = m_carrier->read_reg(I2CM0CONFIG::ADDRESS + m_offset_write);
    config_reg_val &= (~I2CM0CONFIG::I2CM0ADDRESSEXT_BIT_MASK);
    //doing read-modify-write of lpgbt would have been smarter
    mTransactionBuilder.addTransaction( I2CM0CONFIG::ADDRESS + m_offset_write, std::vector<unsigned int>{config_reg_val | address_high << I2CM0CONFIG::I2CM0ADDRESSEXT_OFFSET} );
  }
}

void LPGBT_I2C::master_issue_command(unsigned int command,bool executeNow)
{
  spdlog::apply_all([&](LoggerPtr l) { l->trace("{} : master_issue_command {}",m_name,command); });
  if( m_addr_10bit ){
    auto errorMsg = "master_issue_command is not yet implemented in 10bit addressing mode";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg); });
    throw swamp::LPGBTI2CCommandErrorException(errorMsg);
  }
  
  mTransactionBuilder.addTransaction( I2CM0CMD_ADDRESS + m_offset_write, std::vector<unsigned int>{command} );
  if( std::find( commandToExecute.begin(), commandToExecute.end(), command )!=commandToExecute.end() || executeNow ){
    m_carrier->write_regs(mTransactionBuilder.regs, mTransactionBuilder.vals);
    mTransactionBuilder.clear();
  }
}

I2CTransactionStatus LPGBT_I2C::master_await_completion()
{
  spdlog::apply_all([&](LoggerPtr l) { l->trace("{} : master_await_completion",m_name); });
  boost::timer::cpu_timer timer;
  mTransactionMonitor.incrementCounter("nTransaction");
  while(1){
    auto status = m_carrier->read_reg(I2CM0STATUS_ADDRESS + m_offset_read);

    if( status & I2cmStatusReg::SUCCESS ){
      mTransactionMonitor.incrementCounter("nSuccess");
      return I2cmStatusReg::SUCCESS;
    }

    if( status & I2cmStatusReg::LEVEERR ){
      // raising an exception would be a better ID
      spdlog::apply_all([&](LoggerPtr l) { l->info("{}: The SDA line of I2C master is pulled low before initiating a transaction. The transaction will be tried again.",m_name); });
      mTransactionMonitor.incrementCounter("nSDALow");
      return I2cmStatusReg::LEVEERR;
    }

    if( status & I2cmStatusReg::NOACK ){
      spdlog::apply_all([&](LoggerPtr l) { l->info("{}: The last transaction of I2C master was not acknowledged by the I2C slave. The transaction will be tried again.",m_name); });
      mTransactionMonitor.incrementCounter("nNack");
      return I2cmStatusReg::NOACK;
    }
    
    if( timer.elapsed().wall/1e9 > m_timeout ){
      spdlog::apply_all([&](LoggerPtr l) { l->info("{}: Timeout while waiting for I2C master to finish (status:0x{:02x}). The transaction will be tried again.",m_name,status); });
      mTransactionMonitor.incrementCounter("nTimeout");
      return I2cmStatusReg::TIMEOUT;
    }
  }
}

void LPGBT_I2C::master_set_nbytes(unsigned int nbytes)
{
  /*
    Configures number of bytes used during multi-byte READ and WRITE
    transactions. Implemented as a RMW operation in order to preserve
    I2C master configuration fields.
  */
 spdlog::apply_all([&](LoggerPtr l) { l->trace("{} : master_set_nbytes {}",m_name,nbytes); });
 assert( nbytes<17 );//useless as it is already checked

 auto control_reg_val = m_carrier->read_reg(I2CM0CTRL_ADDRESS + m_offset_read); //read from cache would be better
 control_reg_val &= (~I2cmConfigReg::NBYTES_BIT_MASK);


 mTransactionBuilder.addTransaction( I2CM0DATA0_ADDRESS + m_offset_write, std::vector<unsigned int>{control_reg_val | nbytes << I2cmConfigReg::NBYTES_OFFSET} );
 master_issue_command(I2cmCommand::WRITE_CRA);
}

void LPGBT_I2C::execute_transactions(unsigned int command)
{
  int nattempts(0);
  I2CTransactionStatus status;
  while(nattempts<MAXIMUM_NUMBER_OF_I2C_ATTEMPTS){
    master_issue_command(command);
    status = master_await_completion();
    nattempts++;
    if(status.status==I2cmStatusReg::SUCCESS) break;
  }
  if( status.status!=I2cmStatusReg::SUCCESS ){
    auto errorMsg = "{}: The last I2C transaction did not success after {} attempts.";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name,MAXIMUM_NUMBER_OF_I2C_ATTEMPTS); });
    mTransactionMonitor.incrementCounter("nI2CFailure");
    // NOTE: unlock should not be call here once we will stop exiting the program and rather handle the error properly
    m_carrier->unlock();
    throw swamp::LPGBTI2CTransactionFailureException(errorMsg);
  }
  else
    spdlog::apply_all([&](LoggerPtr l) { l->trace("{}: The last I2C transaction succeed after {} attempts. ",m_name,nattempts); });
}

