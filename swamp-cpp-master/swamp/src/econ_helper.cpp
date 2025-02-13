#include <numeric>

#include "econ_helper.hpp"
#include "generated_econd_register_map.hpp"
#include "generated_econt_register_map.hpp"
#include "magic_enum.hpp"
#include "yaml_helper.hpp"
#include "spdlogger.hpp"

namespace ECON
{
  unsigned int econ_helper::get_lsb_id(unsigned int val)
  {
    return int( std::log2(val&-val) );
  }

  ECONParam econ_helper::getParam(const std::string& pname)
  {
    ECONParam param;
    switch( m_chiptype ){
    case swamp::ChipType::ECON_D :
      {
	auto p = magic_enum::enum_cast<ECOND_PARAM>(pname);
	param = ECONDParams.at( static_cast<int>(p.value()) );
	break;
      }
    case swamp::ChipType::ECON_T :
      {
	auto p = magic_enum::enum_cast<ECONT_PARAM>(pname);
	param = ECONTParams.at( static_cast<int>(p.value()) );
	break;
      }
    default:
      break;
    }
    return param;
  }

  uint32_t econ_helper::getMaxValue(const std::string& pname)
  {
    auto param = getParam(pname);
    return param.param_mask;
  }


  bool econ_helper::checkIfParamInRegMap(const std::string& pname)
  {
    bool inMap(true);
    switch( m_chiptype ){
    case swamp::ChipType::ECON_D :
      {
	auto p = magic_enum::enum_cast<ECOND_PARAM>(pname);
	if( !p.has_value() ){
	  spdlog::apply_all([&](LoggerPtr l) { l->error( "econ_helper::checkIfParamInRegMap: {} was not found in ECOND register map",pname ); });
	  inMap=false;
	}
	else
	  inMap=true;
	break;
      }
    case swamp::ChipType::ECON_T :
      {
	auto p = magic_enum::enum_cast<ECONT_PARAM>(pname);
	if( !p.has_value() ){
	  spdlog::apply_all([&](LoggerPtr l) { l->error( "econ_helper::checkIfParamInRegMap: {} was not found in ECONT register map",pname ); });
	  inMap=false;
	}
	else
	  inMap=true;
	break;
      }
    default:
      break;
    }
    return inMap;
  }
  
  bool econ_helper::validate_configuration(const YAML::Node& node)
  {
    auto str_and_val_vec = yaml_helper::flattenConfig(node);
    for( auto sv : str_and_val_vec ){
      auto pname = sv.mStr;
      auto val = sv.mVal;
      if( checkIfParamInRegMap(pname)==false ){
	spdlog::apply_all([&](LoggerPtr l) { l->critical("Node {} was not found in ECON regmap",pname); });
	return false;
      }
      auto maxVal = getMaxValue(pname);
      if( val>maxVal ){
	spdlog::apply_all([&](LoggerPtr l) { l->critical("Value {} of param {} is greater than allowable maximum {}",val, pname, maxVal); });
	return false;
      }
    }
    return true;
  }

  std::map<uint16_t,uint8_t> econ_helper::defaultRegMap()
  {
    std::map<uint16_t,uint8_t> aMap;
    switch( m_chiptype ){
    case swamp::ChipType::ECON_D :
      {
	for( auto param : ECONDParams ){
	  unsigned int lShift(0);
	  for(auto byteId(0); byteId<param.size_byte; byteId++){
	    uint16_t address = param.address+byteId;
	    auto rShift = byteId == 0 ? param.param_shift : 0;
	    uint8_t value = ( (param.default_value>>lShift) << rShift )&0xFF;
	    if( aMap.find(address)==aMap.end() )
	      aMap.insert( std::pair<uint16_t,uint8_t >(address,value) );
	    else
	      aMap[address]|=value;
	    lShift += 8-rShift;
	  }
	}
	break;
      }
    case swamp::ChipType::ECON_T :
      {
	for( auto param : ECONTParams ){
	  unsigned int lShift(0);
	  for(auto byteId(0); byteId<param.size_byte; byteId++){
	    uint16_t address = param.address+byteId;
	    auto rShift = byteId == 0 ? param.param_shift : 0;
	    uint8_t value = ( (param.default_value>>lShift) << rShift )&0xFF;
	    if( aMap.find(address)==aMap.end() )
	      aMap.insert( std::pair<uint16_t,uint8_t >(address,value) );
	    else
	      aMap[address]|=value;
	    lShift += 8-rShift;
	  }
	}
	break;
      }
    default:
      break;
    }
    return aMap;
  }

  std::set<uint16_t> econ_helper::i2cAddresses() const
  {
    spdlog::apply_all([&](LoggerPtr l) { l->debug( "in econ_helper::i2cAddresses" ); });
    std::set<uint16_t> aVec;
    switch( m_chiptype ){
    case swamp::ChipType::ECON_D :
      {
	for( auto param : ECONDParams ){
	  for(auto byteId(0); byteId<param.size_byte; byteId++){
	    uint16_t address = param.address+byteId;
	    aVec.insert(address);
	  }
	}
	break;
      }
    case swamp::ChipType::ECON_T :
      {
	for( auto param : ECONTParams ){
	  for(auto byteId(0); byteId<param.size_byte; byteId++){
	    uint16_t address = param.address+byteId;
	    aVec.insert(address);
	  }
	}
	break;
      }
    default:
      break;
    }
    return aVec;
  }
  
  YAML::Node econ_helper::regAndVal2Yaml( std::map<uint16_t,uint8_t> reg_and_val_map )
  {
    YAML::Node read_config;
    switch( m_chiptype ){
    case swamp::ChipType::ECON_D :
      {
	std::map<ECOND_PARAM,uint32_t> param_and_val_map;
	unsigned int paramid(0);
	for( auto param : ECONDParams ){
	  auto param_enum = static_cast<ECOND_PARAM>(paramid);
	  uint16_t parShift(0);
	  for(auto byteId(0); byteId<param.size_byte; byteId++){
	    auto regShift = byteId == 0 ? param.param_shift : 0;
	    uint16_t address = param.address+byteId;
	    if( reg_and_val_map.find(address)!=reg_and_val_map.end() ){
	      if( param_and_val_map.find( param_enum )==param_and_val_map.end() )
		param_and_val_map.insert( std::pair<ECOND_PARAM,uint32_t>(param_enum,0) );
	      auto val = (reg_and_val_map[address]>>regShift) & (param.param_mask>>parShift);
	      param_and_val_map[param_enum] |= val << parShift;
	    }
	    parShift += 8-regShift;
	  }
	  paramid++;
	}
	for(auto param_and_val : param_and_val_map ){
	  auto name = magic_enum::enum_name(param_and_val.first);
	  auto strvec = yaml_helper::strToYamlNodeNames(static_cast<std::string>(name));
	  auto block_name = strvec[0];
	  auto subblock_name = strvec[1];
	  auto param_name = strvec[2];
	  if(strvec.size()==4){
	    auto param_id = strvec[3];
	    read_config[block_name][subblock_name][param_name][param_id] = param_and_val.second;
	  }
	  else
	    read_config[block_name][subblock_name][param_name] = param_and_val.second;
	}
	break;
      }
    case swamp::ChipType::ECON_T :
      {
	std::map<ECONT_PARAM,uint32_t> param_and_val_map;
	unsigned int paramid(0);
	for( auto param : ECONTParams ){
	  auto param_enum = static_cast<ECONT_PARAM>(paramid);
	  uint16_t parShift(0);
	  for(auto byteId(0); byteId<param.size_byte; byteId++){
	    auto regShift = byteId == 0 ? param.param_shift : 0;
	    uint16_t address = param.address+byteId;
	    if( reg_and_val_map.find(address)!=reg_and_val_map.end() ){
	      if( param_and_val_map.find( param_enum )==param_and_val_map.end() )
		param_and_val_map.insert( std::pair<ECONT_PARAM,uint32_t>(param_enum,0) );
	      auto val = (reg_and_val_map[address]>>regShift) & (param.param_mask>>parShift);
	      param_and_val_map[param_enum] |= val << parShift;
	    }
	    parShift += 8-regShift;
	  }
	  paramid++;
	}
	for(auto param_and_val : param_and_val_map ){
	  auto name = magic_enum::enum_name(param_and_val.first);
	  auto strvec = yaml_helper::strToYamlNodeNames(static_cast<std::string>(name));
	  auto block_name = strvec[0];
	  auto subblock_name = strvec[1];
	  if(strvec.size()==4){
	    auto param_name = strvec[2];
	    auto param_id = strvec[3];
	    read_config[block_name][subblock_name][param_name][param_id] = param_and_val.second;
	  }
	  else if(strvec.size()==3){
	    auto param_name = strvec[2];
	    read_config[block_name][subblock_name][param_name] = param_and_val.second;
	  }
	  else
	    read_config[block_name][subblock_name] = param_and_val.second;
	}
	break;
      }
    default:
      break;
    }
    return read_config;
  }
}

