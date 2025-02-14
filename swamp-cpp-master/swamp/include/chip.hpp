#pragma once
#include "transport.hpp"

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/vector.hpp>

#include "carrier.hpp"
#include "swamp_type.hpp"

class Chip : public Carrier
{
public:

  Chip( const std::string& name, const YAML::Node& config) : Carrier(name,config)
  {
    std::ostringstream os( std::ostringstream::ate );
    os.str(""); os << config ;
    spdlog::apply_all([&](LoggerPtr l) { l->debug("Creating chip {} with config \n{}",m_name,os.str().c_str()); });

    if( config["address"].IsDefined() )
      m_address = config["address"].as<unsigned int>();
    else{
      auto errorMsg = "address was not found in chip config (in constructor)";
      spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg); });
      throw swamp::ChipConstructorException(errorMsg);
    }

    std::string chipTypeStr = "GENERIC";
    if( config["chipType"].IsDefined() )
      chipTypeStr = config["chipType"].as<std::string>();
    else{
      auto warnMsg = "chipType was not found in chip config (in constructor) -> generic type will be applied (may throw an error later)";
      spdlog::apply_all([&](LoggerPtr l) { l->warn(warnMsg); });
    }
    
    std::transform(chipTypeStr.begin(), chipTypeStr.end(), chipTypeStr.begin(), 
		   [](const char& c){ return std::toupper(c);} );

    std::unordered_map<std::string,swamp::ChipType> typeMap{ {"GENERIC"        ,swamp::ChipType::GENERIC},
							     {"LPGBT"          ,swamp::ChipType::LPGBT},
							     {"LPGBT_D"        ,swamp::ChipType::LPGBT_D},
							     {"LPGBT_T"        ,swamp::ChipType::LPGBT_T},
							     {"LPGBT_D2"       ,swamp::ChipType::LPGBT_D2},
							     {"HGCROC"         ,swamp::ChipType::HGCROC},
							     {"HGCROC_SI"      ,swamp::ChipType::HGCROC_Si},
							     {"HGCROC_SIPM"    ,swamp::ChipType::HGCROC_SiPM},
							     {"ECON"           ,swamp::ChipType::ECON},
							     {"ECON_D"         ,swamp::ChipType::ECON_D},
							     {"ECON_T"         ,swamp::ChipType::ECON_T},
							     {"VTRXP"          ,swamp::ChipType::VTRXP},
							     {"GBT_SCA"        ,swamp::ChipType::GBT_SCA}
    };
    
    if( auto it = typeMap.find( chipTypeStr ); it != typeMap.end() ){
      m_chiptype = it->second;
    }
    else{
      auto errorMsg = "A chiptype ({}) was not found in chip type Map (in Chip constructor)";
      spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,chipTypeStr); });
      throw swamp::ChipConstructorException(errorMsg);      
    }
  }

  inline void setTransport( TransportPtr transport ){ m_transport = transport; }

  inline unsigned int address(){ return m_address; }

  void configure( const YAML::Node& config )
  {
    std::ostringstream os( std::ostringstream::ate );
    os.str(""); os << config ;
    spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : configuring with config \n{}",m_name,os.str().c_str()); });
    lock();
    ConfigureImpl(config);
    unlock();
  }
  
  const YAML::Node read( const YAML::Node& config, bool fromHardware=true )
  {
    std::ostringstream os( std::ostringstream::ate );
    os.str(""); os << config ;
    spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : reading (fromHardware={}) with config \n{}",m_name,fromHardware,os.str().c_str()); });
    lock();
    auto read_config = ReadImpl(config,fromHardware);
    unlock();
    return read_config;
  }

  const YAML::Node read( bool fromHardware=true )
  {
    spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : reading (fromHardware={}) full register space",m_name,fromHardware); });
    lock();
    auto read_config = ReadImpl(fromHardware);
    unlock();
    return read_config;
  }
  
  inline void lock()
  {
    spdlog::apply_all([&](LoggerPtr l) { l->trace("{} : trying to lock ressource",m_name); });
    m_transport->lock();
  }

  inline void unlock()
  {
    spdlog::apply_all([&](LoggerPtr l) { l->trace("{} : unlocking ressource",m_name); });
    m_transport->unlock();
  }

  const swamp::ChipType chipType()
  {
    return m_chiptype;
  }
  
protected:
  
  virtual void ConfigureImpl( const YAML::Node& config ) = 0;
  
  virtual const YAML::Node ReadImpl( const YAML::Node& config, bool fromHardware=true ) = 0;

  virtual const YAML::Node ReadImpl( bool fromHardware=true ) = 0;
  
protected:

  TransportPtr m_transport;
  unsigned int m_address;
  swamp::ChipType m_chiptype;
};

using ChipPtr = std::shared_ptr<Chip>;
