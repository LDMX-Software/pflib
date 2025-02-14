#pragma once
#include "transport.hpp"
#include "hw_interface.hpp"
#include <sstream>
#include <numeric>

class DummyTransport : public Transport
{
public:

  DummyTransport( const std::string& name, const YAML::Node& config) : Transport(name,config)
  {
    m_nWrite=0;
    m_nRead=0;
    m_interface = HWInterface("dummyRessource");
    if( config["read_value"].IsDefined() )
      m_read_value = config["read_value"].as<unsigned int>();
    else
      m_read_value = 0x04; // correct value needed for I2C
    spdlog::apply_all([&](LoggerPtr l) { l->debug("{} (constructor) initialized with read_value = {}",m_name,m_read_value); });
  }

  ~DummyTransport()
  {
    spdlog::apply_all([&](LoggerPtr l) { l->debug("{} (destructor) nWrite, nRead = ({}, {})",m_name,m_nWrite,m_nRead); });
  }

  void write_regs(const unsigned int addr, const unsigned int reg_addr_width, const unsigned int reg_addr, const std::vector<unsigned int>& reg_vals)
  {
    std::ostringstream os( std::ostringstream::ate );
    os.str("");
    std::for_each(reg_vals.begin(), reg_vals.end(), [&](const auto& val){os<<"0x"<<std::hex<<std::setw(2)<<std::setfill('0')<<val<<" ";} );
    spdlog::apply_all([&](LoggerPtr l) { l->trace("{} writing at address 0x{:02x}, with reg_address_with = {}, reg_addr = 0x{:02x}, reg_vals = {}",m_name,addr,reg_addr_width,reg_addr,os.str().c_str()); });
    m_nWrite+=reg_vals.size();
    if( m_carrier ){
      std::cout << m_carrier << std::endl;
      m_carrier->write_regs( reg_addr, reg_vals );
      std::cout << m_carrier << std::endl;
    }
  }
    
  void write_reg(const unsigned int addr, const unsigned int reg_val)
  {
    m_nWrite++;
    spdlog::apply_all([&](LoggerPtr l) { l->trace("{} writing at address 0x{:02x}, with reg_val = 0x{:02x}",m_name,addr,reg_val); });
    if( m_carrier )
      m_carrier->write_reg( addr, reg_val );
  }
  
  unsigned int read_reg(const unsigned int addr)
  {
    m_nRead++;
    unsigned int val=m_read_value;
    spdlog::apply_all([&](LoggerPtr l) { l->trace("{} reading 0x{:02x} from address 0x{:02x}",m_name,val,addr); });
    if( m_carrier )
      val = m_carrier->read_reg( addr );
    return val;
  }
  
  std::vector<unsigned int> read_regs(const unsigned int addr, const unsigned int reg_addr_width, const unsigned int reg_addr, const unsigned int read_len)
  {
    std::vector<unsigned int> vec(read_len,m_read_value);
    spdlog::apply_all([&](LoggerPtr l) { l->trace("{} reading at address 0x{:02x}, with reg_address_with = {}, reg_addr = 0x{:02x}, read_len = {}",m_name,addr,reg_addr_width,reg_addr,read_len); });
    m_nRead+=read_len;
    if( m_carrier )
      vec = m_carrier->read_regs( reg_addr, read_len );
    return vec;
  }

  std::vector<unsigned int> read_regs(const unsigned int addr, const unsigned int read_len)
  {
    std::vector<unsigned int> vec(read_len,0xA8);
    spdlog::apply_all([&](LoggerPtr l) { l->trace("{} reading at address 0x{:02x}, with read_len = {}",m_name,addr,read_len); });
    m_nRead+=read_len;
    if( m_carrier ){
      auto reg_addr=0x0;
      vec = m_carrier->read_regs( reg_addr, read_len );
    }
    return vec;
  }

  void write_regs(const unsigned int addr, const unsigned int reg_addr_width, const std::vector<unsigned int>& reg_addr_vec, const std::vector<std::vector<unsigned int>>& reg_vals_vec) {

    std::ostringstream os( std::ostringstream::ate );
    os.str("");
    for( size_t i(0); i<reg_addr_vec.size(); i++ ){
      os<<"0x"<<std::hex<<std::setw(2)<<std::setfill('0')<<reg_addr_vec[i]<<": ";
      std::for_each(reg_vals_vec[i].begin(), reg_vals_vec[i].end(), [&](const auto& val){os<<"0x"<<std::hex<<std::setw(2)<<std::setfill('0')<<val<<" ";} );
      m_nWrite+=reg_vals_vec[i].size();
      os<<"\t\t";
    }
    spdlog::apply_all([&](LoggerPtr l) { l->trace("{} writing at address 0x{:02x}, with reg_address_with = {}, reg_addresses and vals = {}",m_name,addr,reg_addr_width,os.str().c_str()); });
    // if( m_carrier ){
    //   std::cout << m_carrier << std::endl;
    //   m_carrier->write_regs( reg_addr, reg_vals );
    //   std::cout << m_carrier << std::endl;
    // }
    // m_nWrite++;

  }

  std::vector<unsigned int> read_regs(const unsigned int addr, const unsigned int reg_addr_width, const std::vector<unsigned int>& reg_addr_vec, const std::vector<unsigned int>& read_len_vec) {

    auto fullSize = std::accumulate(read_len_vec.begin(),read_len_vec.end(),0,[](unsigned int sum, const auto& read_len){return sum+read_len;});
    std::vector<unsigned int> ret_vec( fullSize );
    spdlog::apply_all([&](LoggerPtr l) { l->trace("{} reading at address 0x{:02x}, with reg_address_with = {}, read_addr_len = ",m_name,addr,reg_addr_width,fullSize); });

    auto ptr=0;
    for( size_t i(0); i<reg_addr_vec.size(); i++ ){
      auto read_len = read_len_vec[i];
      std::vector<unsigned int> vec(read_len,i&0xFF);
      std::copy(vec.begin(),vec.end(),ret_vec.begin()+ptr);
      ptr+=vec.size();
    }      
    // m_nRead+=read_len;
    // if( m_carrier )
    //   vec = m_carrier->read_regs( reg_addr, read_len );
    return ret_vec;
  }

  void LockImpl() override
  {
    m_interface.lock();
  }

  void UnLockImpl() override
  {
    m_interface.unlock();
  }
  
  void ConfigureImpl(const YAML::Node& node) override{;}

  void ResetImpl() override{;}

private:
  unsigned int m_nWrite;
  unsigned int m_nRead;
  unsigned int m_read_value;
  HWInterface m_interface;
};
 
