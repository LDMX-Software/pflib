#include "vtrxp.hpp"
#include "yaml_helper.hpp"
#include "magic_enum.hpp"
#include "generated_vtrxp_register_map.hpp"

VTRXp::VTRXp( const std::string& name, const YAML::Node& config) : Chip(name,config),
								   mCache( AsicCache(name) )
{
  if( !mCache.existsInCache() )
    buildCache();
  else{
    spdlog::apply_all([&](LoggerPtr l) { l->debug( "{} : cache of already exists",m_name ); });
  }
}

void VTRXp::buildCache()
{
  std::string templateName("vtrxp_template");
  auto cacheCopy = mCache.copyFromTemplate( templateName );
  if( !cacheCopy ){
    spdlog::apply_all([&](LoggerPtr l) { l->debug( "{} : building cache from default register map",m_name ); });
    mCache.set( getDefaultRegMap() );
  }
  else
    spdlog::apply_all([&](LoggerPtr l) { l->debug( "{} : building cache from template {}",m_name, templateName); });
}

const std::map<uint16_t,uint8_t> VTRXp::getDefaultRegMap()
{
  std::map<uint16_t,uint8_t> aMap;
  for( auto param : VTRXP::VTRXpParams ){
    unsigned int lShift(0);
    auto nbytes = param.bit_mask <= 0xFF ? 1 : 4;
    for(auto byteId(0); byteId<nbytes; byteId++){
      uint16_t address = param.address+byteId;
      uint8_t value = (param.default_value>>lShift)&0xFF;
      if( aMap.find(address)==aMap.end() )
	aMap.insert( std::pair<uint16_t,uint8_t >(address,value) );
      else
	aMap[address]|=value;
      lShift += 8;
    }
  }
  // std::for_each(aMap.begin(),aMap.end(),[&](const auto& rv){
  //   spdlog::apply_all([&](LoggerPtr l) { l->debug( "{} : DEFAULT MAP: reg address {:02x}; value {:02x} ",m_name, rv.first, rv.second ); });
  // });
  return aMap;
}


void VTRXp::ConfigureImpl( const YAML::Node& config )
{
  auto reg_and_val_map = i2c_transactions_from_yaml_config(config);
  auto consec_reg_map = optimized_i2c_transactions(reg_and_val_map);
  writeI2C(consec_reg_map);
  updateCache(reg_and_val_map);
}


const YAML::Node VTRXp::ReadImpl( bool fromHardware ) 
{
  YAML::Node readConfig;
  return readConfig;
}  

const YAML::Node VTRXp::ReadImpl( const YAML::Node& config, bool fromHardware ) 
{
  auto reg_and_val_map = i2c_transactions_from_yaml_config(config,true);
  std::map<uint8_t, uint8_t> read_reg_and_val_map;
  if( fromHardware ){
    auto consec_reg_map = optimized_i2c_transactions(reg_and_val_map);
    read_reg_and_val_map = readI2C(consec_reg_map);
    updateCache(reg_and_val_map);
  }
  else
    for( auto reg_val : reg_and_val_map ){
      auto address = reg_val.first;
      auto val = mCache.get(address);
      read_reg_and_val_map[address] = val;
    }
  
  std::for_each(read_reg_and_val_map.begin(),read_reg_and_val_map.end(),[&](const auto& rv){
    spdlog::apply_all([&](LoggerPtr l) { l->debug( "{} : READ: reg address {:02x}; value {:02x} ",m_name, rv.first, rv.second ); });
  });
  updateCache(read_reg_and_val_map);

  auto read_config = regAndVal2Yaml( read_reg_and_val_map );
  std::cout << read_config << std::endl;
  auto ret_config = yaml_helper::vtrx_prune(read_config,config);
  return ret_config;
}

const std::map<uint8_t,uint8_t> VTRXp::i2c_transactions_from_yaml_config(const YAML::Node& config, bool readFlag)
{
  auto str_and_val_vec = yaml_helper::flattenConfig(config);

  std::map<uint8_t,uint8_t> reg_and_val_map;
    
  std::for_each(str_and_val_vec.begin(),str_and_val_vec.end(),[&](const auto& sv){

    auto p = magic_enum::enum_cast<VTRXP::VTRXP_PARAM>(sv.mStr);
    VTRXP::VTRXpParam param = VTRXP::VTRXpParams.at( static_cast<int>(p.value()) );

    spdlog::apply_all([&](LoggerPtr l) { l->debug( "{} : parameter {}; new value {} ",m_name, sv.mStr, sv.mVal ); });

    //in VTRX+, params on more than 1 byte have exactly 4 bytes
    auto nbytes = param.bit_mask <= 0xFF ? 1 : 4;

    uint8_t oldVal(0);
    if( nbytes==1 ){
      if( param.bit_mask!=0xFF && !readFlag ){
	if( reg_and_val_map.find(param.address)==reg_and_val_map.end() )
	  oldVal = mCache.get(param.address);
	else
	  oldVal = reg_and_val_map[param.address];
      }
      uint8_t newVal = (sv.mVal << param.offset) & (param.bit_mask << param.offset);
      spdlog::apply_all([&](LoggerPtr l) { l->debug( "{} : new_val {} , oldVal {}",m_name, newVal,oldVal ); });
      newVal |= (oldVal & (~(param.bit_mask<<param.offset)) );
      reg_and_val_map[param.address] = newVal;
    }
    else //in VTRX+, params on more than 1 byte are either READ ONLY or CLEAR-ON-WRITE (so the value to write does not matter)
      for(auto byteId(0); byteId<nbytes; byteId++){
	reg_and_val_map[param.address+byteId] = 0;
      }
  });

  std::for_each(reg_and_val_map.begin(),reg_and_val_map.end(),[&](const auto& rv){
    spdlog::apply_all([&](LoggerPtr l) { l->debug( "{} : reg address {}; value {:02x} ",m_name, rv.first, rv.second ); });
  });


  return reg_and_val_map;
}

const std::map<uint8_t,std::vector<uint8_t> > VTRXp::optimized_i2c_transactions(const std::map<uint8_t,uint8_t>& reg_and_val_map)
{
  std::map<uint8_t,std::vector<uint8_t> > consec_reg_map;
  std::for_each( reg_and_val_map.rbegin(),
		 reg_and_val_map.rend(),
		 [&](const auto& reg_and_val) {
		   auto address = reg_and_val.first;
		   auto val = reg_and_val.second;
		   if( consec_reg_map.find(address+1)==consec_reg_map.end() ){
		     std::vector<uint8_t> vec(1,val);
		     consec_reg_map.insert( std::pair<uint8_t,std::vector<uint8_t> >(address,vec) );
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
		   spdlog::apply_all([&](LoggerPtr l) { l->debug( "{} : Address = {}, Vector = ({})",m_name, reg_and_vals.first, os.str().c_str() ); });
		 } );
  // }
  return consec_reg_map;
}

YAML::Node VTRXp::regAndVal2Yaml( std::map<uint8_t,uint8_t>& reg_and_val_map )
{
  YAML::Node read_config;

  std::map<VTRXP::VTRXP_PARAM,uint32_t> param_and_val_map;
  unsigned int paramid(0);
  for( auto param : VTRXP::VTRXpParams ){
    auto param_enum = static_cast<VTRXP::VTRXP_PARAM>(paramid);
    uint16_t parShift(0);
    auto nbytes = param.bit_mask <= 0xFF ? 1 : 4;
    for(auto byteId(0); byteId<nbytes; byteId++){
      uint16_t address = param.address+byteId;
      if( reg_and_val_map.find(address)!=reg_and_val_map.end() ){
	if( param_and_val_map.find( param_enum )==param_and_val_map.end() )
	  param_and_val_map.insert( std::pair<VTRXP::VTRXP_PARAM,uint32_t>(param_enum,0) );
	auto val = (reg_and_val_map[address]>>param.offset) & (param.bit_mask>>parShift);
	param_and_val_map[param_enum] |= val << parShift;
      }
      parShift += 8;
    }
    paramid++;
  }
  for(auto param_and_val : param_and_val_map ){
    auto name = magic_enum::enum_name(param_and_val.first);
    auto strvec = yaml_helper::strToYamlNodeNames(static_cast<std::string>(name));

    auto block_name = strvec[0];
    if(strvec.size()==2){
      auto param_name = strvec[1];
      read_config[block_name][param_name] = param_and_val.second;
    }
    else
      read_config[block_name] = param_and_val.second;
  }
  return read_config;
}

void VTRXp::writeI2C(const std::map<uint8_t, std::vector<uint8_t> >& consec_reg_map)
{
  constexpr unsigned int reg_address_width=1;
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

std::map<uint8_t,uint8_t> VTRXp::readI2C(std::map<uint8_t, std::vector<uint8_t> >& consec_reg_map)
{
  std::map<uint8_t,uint8_t> amap;
  constexpr unsigned int reg_address_width=1;
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

void VTRXp::updateCache( const std::map<uint8_t,uint8_t>& reg_and_val_map )
{
  std::map<uint16_t,uint8_t> cached_map;
  std::transform(reg_and_val_map.begin(), reg_and_val_map.end(),
		 std::inserter(cached_map,cached_map.begin()),
		 [](const auto& p){
		   return std::pair<uint16_t,uint8_t>((uint16_t)p.first,p.second);
		 });
  mCache.set(cached_map);
}
