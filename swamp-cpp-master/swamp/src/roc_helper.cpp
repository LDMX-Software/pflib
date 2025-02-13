#include <numeric>
// #include <boost/regex.hpp>
// #include <boost/lexical_cast.hpp>

#include "magic_enum.hpp"
#include "generated_roc_register_map.hpp"
#include "generated_roc_direct_register_map.hpp"
#include "roc_helper.hpp"
#include "spdlogger.hpp"
#include "yaml_helper.hpp"

namespace HGCROC
{
  uint32_t roc_helper::getMaxValue(const std::string& pname)
  {
    auto p = magic_enum::enum_cast<ROCParam>(pname);
    auto regs = RocParams.at( static_cast<int>(p.value()) );
    return std::accumulate( regs.cbegin(), regs.cend(), 0,
			    [](uint32_t sum, const auto& reg){return sum+reg.param_mask;});
  }

  uint32_t roc_helper::getDefaultValue(uint16_t r0, uint16_t r1)
  {
    uint32_t default_value=0;
    for( auto param : RocParams ){
      auto iter = std::find_if( param.cbegin(), param.cend(),
				[&r0,&r1](const auto& reg){return reg.R0==r0 && reg.R1==r1;} );
      if( iter!= param.end() ){
	default_value |= (*iter).defval_mask;
      }
    }
    return default_value;
  }

  uint32_t roc_helper::getDefaultValue(uint16_t address)
  {
    return  getDefaultValue( (address&0xFF), (address>>8) );
  }

  bool roc_helper::checkIfParamInRegMap(const std::string& pname)
  {
    auto p = magic_enum::enum_cast<ROCParam>(pname);
    if( !p.has_value() )
      return false;
    return true;
  }

  bool roc_helper::checkIfParamInDirectRegMap(const std::string& pname)
  {
    auto p = magic_enum::enum_cast<ROCDirectRegEnum>(pname);
    if( !p.has_value() )
      return false;
    return true;
  }
  

  bool roc_helper::validate_configuration( const YAML::Node& config )
  {
    auto str_and_val_vec = yaml_helper::flattenConfig(config);
    for( auto sv : str_and_val_vec ){
      auto pname = sv.mStr;
      auto val = sv.mVal;
      if( checkIfParamInRegMap(pname)==false ){
	spdlog::apply_all([&](LoggerPtr l) { l->critical("Node {} was not found in HGCROC regmap",pname); });
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

  std::array<RocReg,4> roc_helper::getRegs(const std::string& pname)
  {
    auto p = magic_enum::enum_cast<ROCParam>(pname);
    return RocParams.at( static_cast<int>(p.value()) );
  }

  bool roc_helper::isValidReg(const RocReg& reg)
  {
    return reg.R0<=0xFF && reg.R1<=0xFF ;
  }

  std::map<uint16_t,uint8_t> roc_helper::defaultRegMap()
  {
    std::map<uint16_t,uint8_t> aMap;
    for( auto param : HGCROC::RocParams ){
      for( auto reg : param ){
	if( reg.R0<=0xFF && reg.R1<=0xFF ){
	  uint16_t address = (reg.R1<<8)|reg.R0; 
	  aMap[address]=getDefaultValue(address);
	}
      }
    }
    return aMap;
  }
  
  std::set<uint16_t> roc_helper::i2cAddresses() const
  {
    std::set<uint16_t> aVec;
    for( auto param : HGCROC::RocParams ){
      for( auto reg : param ){
	uint16_t address = (reg.R1<<8)|reg.R0;
	aVec.insert(address);
      }
    }
    return aVec;
  }

  YAML::Node roc_helper::regAndVal2Yaml( const std::vector<reg_and_val> &reg_and_val_vec )
  {
    auto get_lsb_id = [](const uint32_t val){ return int( std::log2(val&(-val)) ); };    
    std::map<HGCROC::ROCParam,uint32_t> param_and_val_map;
    uint16_t paramid=0;
    for( auto reg_array : HGCROC::RocParams ){
      auto param = static_cast<HGCROC::ROCParam>(paramid);
      for( auto reg: reg_array ){
	auto r0 = reg.R0;
	auto r1 = reg.R1;
	auto iter = std::find_if( reg_and_val_vec.cbegin(), reg_and_val_vec.cend(),
				  [&r0,&r1](const auto& r){return r.r0()==r0 && r.r1()==r1;} );
	if( iter!=reg_and_val_vec.end() ){
	  if( param_and_val_map.find( param )==param_and_val_map.end() )
	    param_and_val_map.insert( std::pair<HGCROC::ROCParam,uint32_t>(param,0) );
	  auto val = (*iter).val() & reg.reg_mask;
	  val >>= get_lsb_id(reg.reg_mask);
	  val <<= get_lsb_id(reg.param_mask);
	  param_and_val_map[param] |= val;
	}
      }
      paramid++;
    }
    YAML::Node read_config;
    for(auto param_and_val : param_and_val_map ){
      auto strname = magic_enum::enum_name(param_and_val.first);
      auto strvec = yaml_helper::strToYamlNodeNames(static_cast<std::string>(strname),"__" );
      auto block    = strvec[0]; 
      auto subblock = strvec[1]; 
      auto pname    = strvec[2]; 
      read_config[block][subblock][pname] = param_and_val.second;
    }
    return read_config;
  }

  std::map<unsigned int,unsigned int> roc_helper::config2DirectRegisterTransaction(const YAML::Node& config)
  {
    auto get_lsb_id = [](const unsigned int val){ return int( std::log2(val&-val) );};

    std::map<unsigned int,unsigned int> addrAndValMap;
    auto str_and_val_vec = yaml_helper::flattenConfig(config);
    for( auto sv : str_and_val_vec ){
      auto pname = sv.mStr;
      auto val = sv.mVal;
      if( checkIfParamInDirectRegMap(pname)==false ){
	spdlog::apply_all([&](LoggerPtr l) { l->error("Node {} was not found in HGCROC direct register map",pname); });
      }
      else{
	auto p = magic_enum::enum_cast<ROCDirectRegEnum>(pname);
	auto param = RocDirectParams.at( static_cast<int>(p.value()) );
	auto address = param.address;
	val = val & param.param_mask;
	val >>= get_lsb_id(param.param_mask);
	val <<= get_lsb_id(param.reg_mask);
	unsigned int old_val = addrAndValMap.find(address)!=addrAndValMap.end() ? addrAndValMap[address] : 0;
	unsigned int inv_mask = 0xff - param.reg_mask;
	addrAndValMap[address] = (old_val & inv_mask) | val ;
      }
    }
    return addrAndValMap;
  }

  YAML::Node roc_helper::directRegAndVal2Yaml( const std::map<unsigned int,unsigned int> &addrAndValMap )
  {
    auto get_lsb_id = [](const uint32_t val){ return int( std::log2(val&(-val)) ); };    
    std::map<HGCROC::ROCDirectRegEnum,uint32_t> param_and_val_map;
    uint16_t paramid=0;
    for( auto reg : HGCROC::RocDirectParams ){
      auto param_enum = static_cast<HGCROC::ROCDirectRegEnum>(paramid);
      auto addr = reg.address;
      spdlog::apply_all([&](LoggerPtr l) { l->debug("roc_helper : testing if direct register with address {} is in addrAndValMap",addr); });
      auto iter = std::find_if( addrAndValMap.cbegin(), addrAndValMap.cend(),
				[&addr](const auto& addrAndVal){return addrAndVal.first==addr;} );
      if( iter!=addrAndValMap.end() ){
	auto val = iter->second & reg.reg_mask;
	val >>= get_lsb_id(reg.reg_mask);
	val <<= get_lsb_id(reg.param_mask);
	if( param_and_val_map.find( param_enum )==param_and_val_map.end() )
	  param_and_val_map.insert( std::pair<HGCROC::ROCDirectRegEnum,uint32_t>(param_enum,0) );
	param_and_val_map[param_enum] |= val;
	spdlog::apply_all([&](LoggerPtr l) { l->debug("roc_helper : filling direct register at address {} with value {}",addr,param_and_val_map[param_enum]); });
      }
      paramid++;
    }
    YAML::Node read_config;
    for(auto param_and_val : param_and_val_map ){
      auto name = magic_enum::enum_name(param_and_val.first);
      auto strvec = yaml_helper::strToYamlNodeNames(static_cast<std::string>(name));
      auto block_name = strvec[0];
      auto param_name = strvec[1];
      read_config[block_name][param_name] = param_and_val.second;
    }
    return read_config;
  }

}
