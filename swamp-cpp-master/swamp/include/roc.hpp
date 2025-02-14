#pragma once
#include <cmath>

#include "chip.hpp"
#include "redis-asic-cache.hpp"
#include "roc_reg_and_val.hpp"
#include "redis-monitoring.hpp"

constexpr int MaxNumberOfReConfiguration = 100;

class Roc : public Chip
{
public:

  Roc( const std::string& name, const YAML::Node& config );
  
protected:
  void ConfigureImpl( const YAML::Node& config ) override ;
  
  const YAML::Node ReadImpl( bool fromHardware=true ) override ;

  const YAML::Node ReadImpl( const YAML::Node& config, bool fromHardware=true ) override ;

  void ConfigureDirectRegisters( const YAML::Node& config );

  void ConfigureIndirectRegisters( const YAML::Node& config );
  
  const YAML::Node ReadDirectRegisters( const YAML::Node& config );

  const YAML::Node ReadIndirectRegisters( const YAML::Node& config, bool fromHardware=true );

  void SplitConfiguration( const YAML::Node& config, YAML::Node& directRegConfig, YAML::Node& indirectRegConfig);
  
private:
  void buildCache();

  void init_monitoring();

  std::vector<reg_and_val> getFullCache();

  const std::vector<reg_and_val> i2c_transactions_from_yaml_config(const YAML::Node& config, bool readFlag=false);

  const std::vector<reg_and_vals> optimized_i2c_transactions(const std::vector<reg_and_val>& reg_and_val_vec);

  void writeI2C(std::vector<reg_and_vals>& consec_reg_vec);
  
  std::vector<reg_and_val> readI2C(std::vector<reg_and_vals>& reg_and_vals_vec);

  void writeDirectAccessI2C( std::map<unsigned int,unsigned int>& addrAndValMap );

  std::map<unsigned int,unsigned int> readDirectAccessI2C( std::map<unsigned int,unsigned int>& addrAndValMap );

  void updateCache( const std::vector<reg_and_val>& reg_and_val_vec );

  void updateCache( const std::vector<reg_and_vals>& consec_reg_vec );

protected:
  std::pair<unsigned int,unsigned int> m_prev_addr;

  AsicCache mCache;
  
  RedisMonitoring mConfigurationMonitor;
  bool mReadAfterWrite;
};
