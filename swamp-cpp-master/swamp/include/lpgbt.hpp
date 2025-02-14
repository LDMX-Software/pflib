#pragma once
#include <cmath>

#include "chip.hpp"
#include "redis-asic-cache.hpp"
#include "lpgbt_reg_and_val.hpp"

#define LPGBT_REG_ADDRESS_WIDTH 2

class Lpgbt : public Chip
{
public:

  Lpgbt( const std::string& name, const YAML::Node& config );
  
  void write_regs( unsigned int reg_addr, const std::vector<unsigned int>& reg_vals ) override ;

  void write_regs(const std::vector<unsigned int>& reg_addr_vec, const std::vector<std::vector<unsigned int>>& reg_vals_vec) override;

  void write_reg( unsigned int reg_addr, unsigned int reg_val, unsigned int mask=0xFF ) override ;
    
  const std::vector<unsigned int> read_regs( unsigned int reg_addr, unsigned int read_len ) override ;

  const std::vector<unsigned int> read_regs( const std::vector<unsigned int>& reg_addr_vec, const std::vector<unsigned int>& read_len_vec ) override;

  const unsigned int read_reg( unsigned int reg_addr ) override ;
  
protected:

  void ConfigureImpl( const YAML::Node& config ) override ;
  
  const YAML::Node ReadImpl( bool fromHardware=true ) override ;

  const YAML::Node ReadImpl( const YAML::Node& config, bool fromHardware=true ) override ;
  
private:
  void buildCache();
  std::vector<lpgbt_reg_and_val> getFullCache();

  const std::vector<lpgbt_reg_and_val>  yaml_2_reg_and_val(const YAML::Node& config, bool readFlag=false);
  const std::vector<lpgbt_reg_and_vals> reg_and_val_2_consecutive_reg_and_vals(const std::vector<lpgbt_reg_and_val>& reg_and_val_vec);
  
  void write_regs(std::vector<lpgbt_reg_and_vals>& consec_reg_vec);
  std::vector<lpgbt_reg_and_val> read_regs(std::vector<lpgbt_reg_and_vals>& consec_reg_vec);
  
  void updateCache( const std::vector<lpgbt_reg_and_val>& reg_and_val_vec);
  void updateCache( const std::vector<lpgbt_reg_and_vals>& consec_reg_vec);
  void updateCache( const uint16_t reg_address, const std::vector<unsigned int>& vals );

private:
  AsicCache mCache;
};

