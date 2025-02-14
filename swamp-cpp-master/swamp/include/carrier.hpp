#pragma once
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <memory>

#include "spdlogger.hpp"
#include "exception.hpp"

class Carrier
{
public:

  Carrier( const std::string& name, const YAML::Node& config) : m_name(name)
  { ; }

  virtual ~Carrier() = default;

  void setLogger( LoggerPtr logger )
  {
    m_logger=logger;
  }

  std::string name() const { return m_name; }
  
  virtual void write_regs( unsigned int reg_addr, const std::vector<unsigned int>& reg_vals ) {return;};
  
  virtual void write_regs(const std::vector<unsigned int>& reg_addr_vec, const std::vector<std::vector<unsigned int>>& reg_vals_vec) {return;}

  virtual void write_reg( unsigned int reg_addr, unsigned int reg_val, unsigned int mask=0xFF ){return;}
    
  virtual const std::vector<unsigned int> read_regs( unsigned int reg_addr, unsigned int read_len ){return std::vector<unsigned int>();}

  virtual const std::vector<unsigned int> read_regs( const std::vector<unsigned int>& reg_addr_vec, const std::vector<unsigned int>& read_len_vec ){return std::vector<unsigned int>();}

  virtual const unsigned int read_reg( unsigned int reg_addr ){return (unsigned int)(0);}

  virtual void lock() = 0;

  virtual void unlock() = 0;
  
protected:

  std::string m_name;

  LoggerPtr m_logger;
  
};

using CarrierPtr = std::shared_ptr<Carrier>;

