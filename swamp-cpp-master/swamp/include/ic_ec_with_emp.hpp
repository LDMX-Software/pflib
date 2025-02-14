#pragma once
#include "transport.hpp"
#include "emp_interface.hpp"

class ICECWithEMP : public Transport
{
public:

  ICECWithEMP( const std::string& name, const YAML::Node& config);
  
  ~ICECWithEMP()
  {
  }

  void write_regs(const unsigned int addr, const unsigned int reg_addr_width, const unsigned int reg_addr, const std::vector<unsigned int>& reg_vals);
    
  void write_regs(const unsigned int addr, const unsigned int reg_addr_width, const std::vector<unsigned int>& reg_addr_vec, const std::vector<std::vector<unsigned int>>& reg_vals_vec);

  void write_reg(const unsigned int addr, const unsigned int reg_val);
  
  unsigned int read_reg(const unsigned int addr);
  
  std::vector<unsigned int> read_regs(const unsigned int addr, const unsigned int reg_addr_width, const unsigned int reg_addr, const unsigned int read_len);

  std::vector<unsigned int> read_regs(const unsigned int addr, const unsigned int reg_addr_width, const std::vector<unsigned int>& reg_addr_vec, const std::vector<unsigned int>& read_len_vec);
  
protected:
  void LockImpl() override;

  void UnLockImpl() override;

  void ConfigureImpl(const YAML::Node& node) override{;}

  void ResetImpl() override{;}

private:
  EMPInterface m_emp_interface;
  bool m_isIC;
};
 
