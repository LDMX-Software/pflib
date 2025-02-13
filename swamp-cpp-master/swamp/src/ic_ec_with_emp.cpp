#include "ic_ec_with_emp.hpp"
#include <sstream>

#include <sys/utsname.h>

ICECWithEMP::ICECWithEMP( const std::string& name, const YAML::Node& config) : Transport(name,config)
{
  std::string connection_file = std::string("file:///home/cmx/rshukla/test_stand/connections.xml");
  if( config["connection_file"].IsDefined() )
    connection_file = config["connection_file"].as<std::string>();

  std::string device = std::string("x0");
  if( config["device"].IsDefined() )
    device = config["device"].as<std::string>();
    
  unsigned int emp_channel = 13;
  if( config["emp_channel"].IsDefined() )
    emp_channel = config["emp_channel"].as<unsigned int>();
    
  unsigned int timeout = 10000;
  if( config["timeout"].IsDefined() )
    timeout = config["timeout"].as<unsigned int>();

  m_isIC = true;
  if( config["isIC"].IsDefined() )
    m_isIC = config["isIC"].as<bool>();
    
  struct utsname sysinfo;
  uname(&sysinfo);
  std::ostringstream os(std::ostringstream::ate);
  os.str("");
  os << sysinfo.nodename << ":emptransactor0";  

  m_emp_interface = EMPInterface(connection_file,
				 device,
				 emp_channel,
				 timeout,
				 os.str());
}

void ICECWithEMP::write_regs(const unsigned int addr, const unsigned int reg_addr_width, const unsigned int reg_addr, const std::vector<unsigned int>& reg_vals)
{
  // reg_address_width is not used here but we keep it to have same interface with other transports (e.g. I2C transports)
  std::ostringstream os( std::ostringstream::ate );
  os.str("");
  std::for_each(reg_vals.begin(), reg_vals.end(), [&](const auto& val){os<<"0x"<<std::hex<<std::setw(2)<<std::setfill('0')<<val<<" ";} );
  spdlog::apply_all([&](LoggerPtr l) { l->trace("IC/EC EMP: writing at address 0x{:02x}, with red_address_with = {}, reg_addr = 0x{:02x}, reg_vals = {}",addr,reg_addr_width,reg_addr,os.str().c_str()); });

  if(m_isIC)
    m_emp_interface.set_IC();
  else
    m_emp_interface.set_EC();

  m_emp_interface.write_IC(reg_addr, reg_vals, addr);
  //we may need to split the transactions if we encounter emp timeout issues, see write_regs/read_regs functions in : https://gitlab.cern.ch/hgcal-daq-sw/swamp/-/blob/train_v3/lpgbt_ic_emp.py?ref_type=heads#L24-65
}

void ICECWithEMP::write_regs(const unsigned int addr, const unsigned int reg_addr_width, const std::vector<unsigned int>& reg_addr_vec, const std::vector<std::vector<unsigned int>>& reg_vals_vec)
{
  // reg_address_width is not used here but we keep it to have same interface with other transports (e.g. I2C transports)
  std::ostringstream os( std::ostringstream::ate );
  os.str("");
  std::for_each(reg_addr_vec.begin(), reg_addr_vec.end(), [&](const auto& val){os<<"0x"<<std::hex<<std::setw(2)<<std::setfill('0')<<val<<" ";} );
  spdlog::apply_all([&](LoggerPtr l) { l->debug("IC/EC EMP: writing at address 0x{:02x}, with red_address_with = {}, reg_addresses = {}",addr,reg_addr_width,os.str().c_str()); });

  if(m_isIC)
    m_emp_interface.set_IC();
  else
    m_emp_interface.set_EC();

  m_emp_interface.blockWrite_IC(reg_addr_vec, reg_vals_vec, addr);
  //we may need to split the transactions if we encounter emp timeout issues, see write_regs/read_regs functions in : https://gitlab.cern.ch/hgcal-daq-sw/swamp/-/blob/train_v3/lpgbt_ic_emp.py?ref_type=heads#L24-65
}

void ICECWithEMP::write_reg(const unsigned int addr, const unsigned int reg_val)
{
  auto errorMsg = "IC/EC EMP: write_reg is not implemented, please use write_regs methods instead";
  spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg); });
  throw swamp::MethodNotImplementedException(errorMsg);
  write_regs(addr,0x1,0x0,std::vector<unsigned int>(reg_val,1));
}
  
unsigned int ICECWithEMP::read_reg(const unsigned int addr)
{
  auto errorMsg = "IC/EC EMP: read_reg is not implemented, please use write_regs methods instead";
  spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg); });
  throw swamp::MethodNotImplementedException(errorMsg);
  read_regs(addr,0x1,0x0,0x1);
  return 0;
}
  
std::vector<unsigned int> ICECWithEMP::read_regs(const unsigned int addr, const unsigned int reg_addr_width, const unsigned int reg_addr, const unsigned int read_len)
{
  // reg_address_width is not used here but we keep it to have same interface with other transports (e.g. I2C transports)
  spdlog::apply_all([&](LoggerPtr l) { l->trace("IC/EC EMP: reading at address 0x{:02x}, with red_address_with = {}, reg_addr = 0x{:02x}, reg_len = {}",addr,reg_addr_width,reg_addr,read_len); });
  if(m_isIC)
    m_emp_interface.set_IC();
  else
    m_emp_interface.set_EC();

  auto vec = m_emp_interface.read_IC(reg_addr, read_len, addr);
  std::transform( vec.begin(), 
		  vec.end(), 
		  vec.begin(),
		  [](auto &v){ return v&0xFF;} );
  return vec;
}

std::vector<unsigned int> ICECWithEMP::read_regs(const unsigned int addr, const unsigned int reg_addr_width, const std::vector<unsigned int>& reg_addr_vec, const std::vector<unsigned int>& read_len_vec)
{
  // reg_address_width is not used here but we keep it to have same interface with other transports (e.g. I2C transports)
  std::ostringstream os( std::ostringstream::ate );
  os.str("");
  std::for_each(reg_addr_vec.begin(), reg_addr_vec.end(), [&](const auto& val){os<<"0x"<<std::hex<<std::setw(2)<<std::setfill('0')<<val<<" ";} );
  spdlog::apply_all([&](LoggerPtr l) { l->debug("IC/EC EMP: read at address 0x{:02x}, with red_address_with = {}, reg_addresses = {}",addr,reg_addr_width,os.str().c_str()); });

  if(m_isIC)
    m_emp_interface.set_IC();
  else
    m_emp_interface.set_EC();

  auto vec = m_emp_interface.blockRead_IC(reg_addr_vec, read_len_vec, addr);
  std::transform( vec.begin(), 
		  vec.end(), 
		  vec.begin(),
		  [](auto &v){ return v&0xFF;} );
  return vec;
}


void ICECWithEMP::LockImpl()
{
  m_emp_interface.lock();
}

void ICECWithEMP::UnLockImpl()
{
  m_emp_interface.unlock();
}

