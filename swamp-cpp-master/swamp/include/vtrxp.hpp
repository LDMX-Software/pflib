#pragma once
#include <cmath>

#include "chip.hpp"
#include "redis-asic-cache.hpp"

class VTRXp : public Chip
{
public:

  VTRXp( const std::string& name, const YAML::Node& config );
  
protected:
  void ConfigureImpl( const YAML::Node& config ) override ;
  
  const YAML::Node ReadImpl( bool fromHardware=true ) override ;

  const YAML::Node ReadImpl( const YAML::Node& config, bool fromHardware=true ) override ;

private:

  void buildCache();

  const std::map<uint16_t,uint8_t> getDefaultRegMap();

  const std::map<uint8_t,uint8_t> i2c_transactions_from_yaml_config(const YAML::Node& config, bool readFlag=false);
  
  const std::map<uint8_t,std::vector<uint8_t> > optimized_i2c_transactions(const std::map<uint8_t,uint8_t>& reg_and_val_map);

  YAML::Node regAndVal2Yaml( std::map<uint8_t,uint8_t>& reg_and_val_map );

  void writeI2C(const std::map<uint8_t,std::vector<uint8_t> >& reg_and_val_map);

  std::map<uint8_t,uint8_t> readI2C(std::map<uint8_t, std::vector<uint8_t> >& consec_reg_map);

  void updateCache( const std::map<uint8_t,uint8_t>& reg_and_val_map );

protected:
 AsicCache mCache;
};
