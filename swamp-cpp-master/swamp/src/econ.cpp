#include "econ.hpp"
#include <stdio.h>
#include <econ_helper.hpp>
#include <yaml_helper.hpp>

using namespace ECON;


Econ::Econ( const std::string& name, const YAML::Node& config) : Chip(name,config),
								 mCache( AsicCache(name) )
{
  if( (m_address&0xF0)==0x20 && m_chiptype != swamp::ChipType::ECON_T){
    auto errorMsg = "An ECON ({}) should be defined as ECON_T with address 0x2X but was defined as something else";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name); });
    throw swamp::ECONConstructorErrorException(errorMsg);
  }
  else if( (m_address&0xF0)==0x60 && m_chiptype != swamp::ChipType::ECON_D ){
    auto errorMsg = "An ECON ({}) should be defined as ECON_D with address 0x6X but was defined as something else";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name); });
    throw swamp::ECONConstructorErrorException(errorMsg);
  }

  if( m_chiptype == swamp::ChipType::ECON_D )
    spdlog::apply_all([&](LoggerPtr l) { l->debug("{} will be defined as ECON-D",m_name); });
  else
    spdlog::apply_all([&](LoggerPtr l) { l->debug("{} will be defined as ECON-T",m_name); });

  if( !mCache.existsInCache() )
    buildCache();
  else{
    spdlog::apply_all([&](LoggerPtr l) { l->debug( "{} : cache of already exists",m_name ); });
  }
}

void Econ::buildCache()
{
  std::string templateName;
  switch( m_chiptype ){
  case swamp::ChipType::ECON_D :
    {
      templateName = std::string("econd_template");
      break;
    }
  case swamp::ChipType::ECON_T :
    {
      templateName = std::string("econt_template");
      break;
    }
  default:
    break;
  }
  auto cacheCopy = mCache.copyFromTemplate( templateName );
  if( !cacheCopy ){
    spdlog::apply_all([&](LoggerPtr l) { l->debug( "{} : building cache from default register map",m_name ); });
    auto helper = ECON::econ_helper(m_chiptype);
    mCache.set(helper.defaultRegMap());
  }
  else
    spdlog::apply_all([&](LoggerPtr l) { l->debug( "{} : building cache from template {}",m_name, templateName); });
}

std::map<uint16_t,uint8_t> Econ::getFullCache()
{
  spdlog::apply_all([&](LoggerPtr l) { l->debug( "{} : getting full cache",m_name ); });
  auto helper = ECON::econ_helper(m_chiptype);
  auto addresses = helper.i2cAddresses();
  return mCache.get(addresses);
}

void Econ::ConfigureImpl( const YAML::Node& config )
{
  auto helper = ECON::econ_helper(m_chiptype);
  if( !helper.validate_configuration( config ) ){
    auto errorMsg = "{} : wrong format for configuration, this chip will not be configured";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name); });
    throw swamp::ECONYAMLConfigErrorException(errorMsg);
  }
  auto reg_and_val_map = i2c_transactions_from_yaml_config(config);
  if( !reg_and_val_map.size() ) return;
  auto consec_reg_map = optimized_i2c_transactions(reg_and_val_map);
  writeI2C(consec_reg_map);
  updateCache(reg_and_val_map);
}

const YAML::Node Econ::ReadImpl( const YAML::Node& config, bool fromHardware )
{
  YAML::Node read_config;

  std::ostringstream os( std::ostringstream::ate );
  os.str(""); os << config ;
  spdlog::apply_all([&](LoggerPtr l) { l->debug("{} read config with config \n{}",m_name,os.str().c_str()); });

  auto helper = ECON::econ_helper(m_chiptype);
  if( !helper.validate_configuration( config ) ){
    auto errorMsg = "{} : wrong format for configuration, reading configuration from this chip will not happen";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name); });
    throw swamp::ECONYAMLConfigErrorException(errorMsg);
  }

  auto reg_and_val_map = i2c_transactions_from_yaml_config(config,true);
  std::map<uint16_t, uint8_t> read_reg_and_val_map;
  if( fromHardware ){
    auto consec_reg_map = optimized_i2c_transactions(reg_and_val_map);
    read_reg_and_val_map = readI2C(consec_reg_map);
    updateCache(reg_and_val_map);
  }
  else{
    for( auto reg_val : reg_and_val_map ){
      auto address = reg_val.first;
      auto val = mCache.get(address);
      read_reg_and_val_map[address] = val;
    }
  }
  read_config = helper.regAndVal2Yaml( read_reg_and_val_map );
  auto ret_config = yaml_helper::prune(read_config,config);
  return ret_config;
}

const YAML::Node Econ::ReadImpl( bool fromHardware ) 
{
  auto helper = ECON::econ_helper(m_chiptype);
  std::map<uint16_t, uint8_t> reg_and_val_cache = getFullCache();
  std::map<uint16_t, uint8_t> reg_and_val_map;
  if( fromHardware ){
    auto consec_reg_map = optimized_i2c_transactions(reg_and_val_cache);
    reg_and_val_map = readI2C(consec_reg_map);
    updateCache(reg_and_val_map);
  }
  else
    reg_and_val_map = reg_and_val_cache;
  YAML::Node read_config = helper.regAndVal2Yaml( reg_and_val_map );
  return read_config;
}  

const std::map<uint16_t,uint8_t> Econ::i2c_transactions_from_yaml_config(const YAML::Node& config, bool readFlag)
{
  auto str_and_val_vec = yaml_helper::flattenConfig(config);

  std::map<uint16_t,uint8_t> reg_and_val_map;
  auto helper = ECON::econ_helper(m_chiptype);
    
  std::for_each(str_and_val_vec.begin(),str_and_val_vec.end(),[&](const auto& sv){
    auto param = helper.getParam(sv.mStr);
    spdlog::apply_all([&](LoggerPtr l) { l->trace( "{} : parameter {}; new value {} ",m_name, sv.mStr, sv.mVal ); });
    unsigned int lShift(0);
    for(auto byteId(0); byteId<param.size_byte; byteId++){
      auto rShift = byteId == 0 ? param.param_shift : 0;
      uint8_t newVal = ((sv.mVal >> lShift)<<rShift) & 0xFF;
      uint8_t bitMask = uint8_t( (param.param_mask<<rShift)>>lShift );
      uint8_t oldVal(0);
      if( bitMask!=0xFF && !readFlag ){
	if( reg_and_val_map.find(param.address+byteId)==reg_and_val_map.end() )
	  oldVal = mCache.get(param.address+byteId);
	else
	  oldVal = reg_and_val_map[param.address+byteId];
	newVal = (oldVal & (~bitMask) ) | newVal;
      }
      spdlog::apply_all([&](LoggerPtr l) { l->trace( "{} : parameter address = {}; old value (0 can be wrong if read mode) = {}; new value = {} ", m_name, param.address+byteId, int(oldVal), int(newVal) ); });
      reg_and_val_map[param.address+byteId] = newVal;
      lShift += 8-rShift;
    }
  });
  return reg_and_val_map;
}

const std::map<uint16_t,std::vector<uint8_t> > Econ::optimized_i2c_transactions(const std::map<uint16_t,uint8_t>& reg_and_val_map)
{
  std::map<uint16_t,std::vector<uint8_t> > consec_reg_map;
  std::for_each( reg_and_val_map.rbegin(),
		 reg_and_val_map.rend(),
		 [&](const auto& reg_and_val) {
		   auto address = reg_and_val.first;
		   auto val = reg_and_val.second;
		   if( consec_reg_map.find(address+1)==consec_reg_map.end() ){
		     std::vector<uint8_t> vec(1,val);
		     consec_reg_map.insert( std::pair<uint16_t,std::vector<uint8_t> >(address,vec) );
		   }
		   else{
		     auto node_handler = consec_reg_map.extract(address+1);
		     node_handler.key() = address; //update the address (basically new_address = old_address-1)
		     consec_reg_map.insert( std::move(node_handler) );
		     consec_reg_map[address].insert(consec_reg_map[address].begin(),val); 
		   }
		 } );
  // if ( spdlog::get_level() == spdlog::level::level_enum::debug ){
  std::for_each( consec_reg_map.begin(),
		 consec_reg_map.end(),
		 [&](const auto& reg_and_vals) {
		   std::ostringstream os(std::ostringstream::ate);
		   os.str("");
		   for(auto val: reg_and_vals.second)
		     os << int(val) << ", " ;
		   spdlog::apply_all([&](LoggerPtr l) { l->trace( "{} : Address = {}, Vector = ({})",m_name, reg_and_vals.first, os.str().c_str() ); });
		 } );
  // }
  return consec_reg_map;
}

void Econ::writeI2C(const std::map<uint16_t, std::vector<uint8_t> >& consec_reg_map)
{
  unsigned int reg_address_width=2;
  std::for_each( consec_reg_map.begin(),
  		 consec_reg_map.end(),
  		 [&](auto &consec_reg){
		   auto reg_addr=consec_reg.first;
		   std::vector<unsigned int> reg_vals; reg_vals.reserve(consec_reg.second.size());
		   std::transform(consec_reg.second.begin(),consec_reg.second.end(),
				  std::back_inserter(reg_vals),
				  [](auto& v){
				    return (unsigned int)v;
				  });
		   m_transport->write_regs(m_address,reg_address_width,reg_addr,reg_vals);
  		 } );
}
  
std::map<uint16_t,uint8_t> Econ::readI2C(std::map<uint16_t, std::vector<uint8_t> >& consec_reg_map)
{
  std::map<uint16_t,uint8_t> amap;
  unsigned int reg_address_width=2;
  std::for_each( consec_reg_map.begin(),
  		 consec_reg_map.end(),
  		 [&](auto &consec_reg){
		   auto reg_addr=consec_reg.first;
		   auto read_len=consec_reg.second.size();
		   auto vec = m_transport->read_regs(m_address,reg_address_width,reg_addr,read_len);
		   std::transform(vec.begin(), vec.end(), std::inserter(amap, amap.end()),
				  [&reg_addr](const auto &v) {
				    auto pair = std::make_pair(reg_addr, v);
				    reg_addr+=1;
				    return pair;
				  });
  		 } );
  return amap;
}

void Econ::updateCache( const std::map<uint16_t,uint8_t>& reg_and_val_vec )
{
  mCache.set(reg_and_val_vec);
}
