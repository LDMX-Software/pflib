#pragma once
#include "chip.hpp"
#include <sstream>

class DummyChip : public Chip
{
public:

  DummyChip( const std::string& name, const YAML::Node& config) : Chip(name,config)
  {
    m_nWrite=0;
    m_nRead=0;    
  }

  ~DummyChip()
  {
    spdlog::apply_all([&](LoggerPtr l) { l->debug("Dummy chip (nWrite, nRead) = ({}, {})",m_nWrite,m_nRead); });
  }

  void write_regs( unsigned int reg_addr, const std::vector<unsigned int>& reg_vals )
  {
    std::ostringstream os( std::ostringstream::ate );
    os.str("");
    std::for_each(reg_vals.begin(), reg_vals.end(), [&](const auto& val){os<<"0x"<<std::hex<<std::setw(2)<<std::setfill('0')<<val<<" ";} );
    spdlog::apply_all([&](LoggerPtr l) { l->debug("Dummy chip writing at reg_address = 0x{:02x}, reg_vals = {}",reg_addr,os.str().c_str()); });
    m_nWrite+=reg_vals.size();
  }
    
  void write_reg(const unsigned int reg_addr, unsigned int reg_val, unsigned int mask=0xFF)
  {
    m_nWrite++;
    spdlog::apply_all([&](LoggerPtr l) { l->debug("Dummy chip writing at reg_address 0x{:02x}, with reg_val = 0x{:02x}",reg_addr,reg_val); });
  }
  
  const unsigned int read_reg( unsigned int reg_addr )
  {
    m_nRead++;
    unsigned int val=0xCD;
    spdlog::apply_all([&](LoggerPtr l) { l->debug("Dummy chip reading 0x{:02x} from reg_address 0x{:02x}",val,reg_addr); });
    return val;
  }
  
  const std::vector<unsigned int> read_regs(unsigned int reg_addr, unsigned int read_len)
  {
    std::vector<unsigned int> vec(read_len,0xB0);
    spdlog::apply_all([&](LoggerPtr l) { l->debug("Dummy chip reading at reg_address = 0x{:02x}, reg_len = {}",reg_addr,read_len); });
    m_nRead+=read_len;
    return vec;
  }
  
protected:

  void ConfigureImpl( const YAML::Node& config ) override
  {
    std::ostringstream os( std::ostringstream::ate );
    os.str(""); os << config ;
    spdlog::apply_all([&](LoggerPtr l) { l->info("Configuring chip {} with config \n{}",m_name,os.str().c_str()); });    
  }

  const YAML::Node ReadImpl( const YAML::Node& config, bool fromHardware=true ) override
  {
    YAML::Node node;
    return node;
  }

  const YAML::Node ReadImpl( bool fromHardware=true ) override
  {
    YAML::Node node;
    return node;
  }
  
private:
  unsigned int m_nWrite;
  unsigned int m_nRead;
};
 
