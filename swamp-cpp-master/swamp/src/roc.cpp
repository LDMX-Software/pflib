#include "roc.hpp"
#include <stdio.h>
#include <roc_helper.hpp>
#include "yaml_helper.hpp"

unsigned int get_lsb_id(unsigned int val)
{
  return int( std::log2(val&-val) );
}

inline void updateVal( reg_and_val &regval, const HGCROC::RocReg& reg, const unsigned int prev_val, const unsigned int new_val)
{
  // Convert parameter value (from config) into register value (1 byte).
  unsigned int val = new_val & reg.param_mask;
  val >>= get_lsb_id(reg.param_mask);
  val <<= get_lsb_id(reg.reg_mask);
  unsigned int inv_mask = 0xff - reg.reg_mask;
  regval.updateVal( (prev_val & inv_mask) | val );
  spdlog::apply_all([&](LoggerPtr l) { l->debug( "init val = {}, prev_val = {}, new_val = {}, param_mask = {}, reg mask = {}, inv_mask = {}, m_val = {}",val, prev_val, new_val,reg.param_mask,reg.reg_mask,inv_mask,regval.val() ); });
}

Roc::Roc( const std::string& name, const YAML::Node& config) : Chip(name,config),
							       mCache( AsicCache(name) )
{
  if( !mCache.existsInCache() )
    buildCache();
  else{
    spdlog::apply_all([&](LoggerPtr l) { l->debug( "Cache of {} already exists",m_name ); });
  }

  mReadAfterWrite=false;
  if( config["readAfterWrite"].IsDefined() )
      mReadAfterWrite = config["readAfterWrite"].as<bool>();
  init_monitoring();
}

void Roc::buildCache()
{
  std::string templateName("siroc_template");
  auto cacheCopy = mCache.copyFromTemplate( templateName );
  if( !cacheCopy ){
    spdlog::apply_all([&](LoggerPtr l) { l->debug( "{} : building cache from default register map",m_name ); });
    auto helper = HGCROC::roc_helper();
    mCache.set(helper.defaultRegMap());
  }
  else
    spdlog::apply_all([&](LoggerPtr l) { l->debug( "{} : building cache from template {}",m_name, templateName); });
}

void Roc::init_monitoring()
{
  std::unordered_map<std::string,uint32_t> counterMap{ {"nConfiguration"        ,0},
						       {"nSuccess"              ,0},
						       {"nRetry"                ,0},
						       {"nFailure"         ,0}
  };
  mConfigurationMonitor = RedisMonitoring(m_name+std::string("_mon"),counterMap);
}

std::vector<reg_and_val> Roc::getFullCache()
{
  auto helper = HGCROC::roc_helper();
  auto addresses = helper.i2cAddresses();
  auto cache_map = mCache.get(addresses);
  std::vector<reg_and_val> reg_val_vec;
  std::transform(cache_map.begin(),cache_map.end(),
		 std::back_inserter(reg_val_vec),
		 [](const auto& p){
		   auto val = p.second;
		   auto address = p.first;
		   auto r0 = address&0xFF;
		   auto r1 = (address>>8)&0xFF;
		   return reg_and_val(r0,r1,val);
		 });
  return reg_val_vec;
}

void Roc::SplitConfiguration( const YAML::Node& config, YAML::Node& directRegConfig, YAML::Node& indirectRegConfig)
{
  if( config["DirectAccess"].IsDefined() ){
    directRegConfig["DirectAccess"]=config["DirectAccess"];
    indirectRegConfig = YAML::Clone(config);
    indirectRegConfig.remove("DirectAccess");
  }
  else
    indirectRegConfig=config;
}

void Roc::ConfigureImpl( const YAML::Node& config )
{
  YAML::Node directRegConfig;
  YAML::Node indirectRegConfig;
  SplitConfiguration( config, directRegConfig, indirectRegConfig);
  ConfigureDirectRegisters(directRegConfig);
  ConfigureIndirectRegisters(indirectRegConfig);
}

void Roc::ConfigureDirectRegisters( const YAML::Node& config )
{
  auto helper = HGCROC::roc_helper();
  auto addrAndValMap = helper.config2DirectRegisterTransaction(config);
  std::for_each(addrAndValMap.cbegin(),
		addrAndValMap.cend(),
		[&](const auto& addAndVal){
		  spdlog::apply_all([&](LoggerPtr l) { l->trace("{} : configure direct register address {} with value {}",m_name,addAndVal.first,addAndVal.second); });
		});
  writeDirectAccessI2C(addrAndValMap);
}

void Roc::ConfigureIndirectRegisters( const YAML::Node& config )
{
  auto helper = HGCROC::roc_helper();
  if( !helper.validate_configuration(config) ){
    auto errorMsg = "{} : wrong format for configuration, this chip will not be configured";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name); });
    throw swamp::ROCYAMLConfigErrorException(errorMsg);
  }
  auto reg_vec = i2c_transactions_from_yaml_config(config);
  if( !reg_vec.size() ) return;

  auto consec_reg_vec = optimized_i2c_transactions(reg_vec);

  int nConfig=0;
  bool readBackOk=true;

  while( nConfig<MaxNumberOfReConfiguration ){
    mConfigurationMonitor.incrementCounter("nConfiguration");
    writeI2C(consec_reg_vec);
    nConfig++;
    if( mReadAfterWrite ){
      auto reg_and_val_vec = readI2C(consec_reg_vec);
      std::sort( reg_and_val_vec.rbegin(), reg_and_val_vec.rend() );
      readBackOk = std::equal( reg_vec.begin(), reg_vec.end(), reg_and_val_vec.begin(),
			       [](const auto& w,const auto& r){
				 return w.r0()==r.r0() && w.r1()==r.r1() && w.val()==r.val();
			       } );
      if( readBackOk==true ) {
	spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : read back OK after configuration",m_name); });
	break;
      }
      else{
	mConfigurationMonitor.incrementCounter("nRetry");
	spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : error in read back -> will try to configure again",m_name); });
      }
    }
    else
      break;
  }

  if( readBackOk ){
    updateCache(reg_vec);
    mConfigurationMonitor.incrementCounter("nSuccess");
    spdlog::apply_all([&](LoggerPtr l) { l->info("{} : configuration success after {} tries",m_name,nConfig); });
  }
  else{
    mConfigurationMonitor.incrementCounter("nFailure");
    spdlog::apply_all([&](LoggerPtr l) { l->error("{} : configuration failure after {} tries",m_name,nConfig); });
  }
}

const YAML::Node Roc::ReadImpl( bool fromHardware ) 
{
  std::vector<reg_and_val> reg_and_val_cache = getFullCache();
  std::vector<reg_and_val> reg_and_val_vec;
  if( fromHardware ){
    auto consec_reg_vec = optimized_i2c_transactions(reg_and_val_cache);
    reg_and_val_vec = readI2C(consec_reg_vec);
    updateCache(reg_and_val_vec);
  }
  else
    reg_and_val_vec = reg_and_val_cache;
  auto helper = HGCROC::roc_helper();  
  YAML::Node read_config = helper.regAndVal2Yaml( reg_and_val_vec );
  return read_config;
}  

const YAML::Node Roc::ReadImpl( const YAML::Node& config, bool fromHardware ) 
{
  YAML::Node directRegConfig;
  YAML::Node indirectRegConfig;
  SplitConfiguration( config, directRegConfig, indirectRegConfig);
  auto readDC = directRegConfig.size()>0 ? ReadDirectRegisters(directRegConfig) : YAML::Node();
  auto readIC = indirectRegConfig.size()>0 ? ReadIndirectRegisters(indirectRegConfig,fromHardware) : YAML::Node();
  return yaml_helper::merge_nodes(readDC,readIC);
}

const YAML::Node Roc::ReadDirectRegisters( const YAML::Node& config )
{
  auto helper = HGCROC::roc_helper();
  auto addrAndValMap = helper.config2DirectRegisterTransaction(config);
  std::for_each(addrAndValMap.cbegin(),
		addrAndValMap.cend(),
		[&](const auto& addAndVal){
		  spdlog::apply_all([&](LoggerPtr l) { l->trace("{} : read direct register address {}",m_name,addAndVal.first); });
		});
  auto readMap = readDirectAccessI2C(addrAndValMap);
  // std::for_each( readMap.begin(),
  // 		 readMap.end(),
  // 		 [](const auto& p){
  // 		   std::cout << p.first << " " << p.second << std::endl;
  // 		 });

  auto node = helper.directRegAndVal2Yaml(readMap);
  return node;
}

const YAML::Node Roc::ReadIndirectRegisters( const YAML::Node& config, bool fromHardware )
{
  auto helper = HGCROC::roc_helper();  
  std::ostringstream os( std::ostringstream::ate );
  os.str(""); os << config ;
  spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : reading of indirect registers with config \n{}",m_name,os.str().c_str()); });
  if( !helper.validate_configuration(config) ){
    auto errorMsg = "{} : wrong format for configuration, reading configuration from this chip will not happen";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name); });
    throw swamp::ROCYAMLConfigErrorException(errorMsg);
  }

  auto reg_vec = i2c_transactions_from_yaml_config(config,true);
  std::vector<reg_and_val> reg_and_val_vec;
  if( fromHardware ){
    auto consec_reg_vec = optimized_i2c_transactions(reg_vec);
    reg_and_val_vec = readI2C(consec_reg_vec);
    updateCache(reg_and_val_vec);
  }
  else{
    for( auto reg_val : reg_vec ){
      unsigned int r0 = reg_val.r0();
      unsigned int r1 = reg_val.r1();
      auto val = mCache.get(reg_val.address());
      reg_and_val_vec.push_back( reg_and_val(r0, r1,val) );
    }
  }

  // std::for_each( reg_and_val_vec.cbegin(), reg_and_val_vec.cend(), [](const auto& regval){ std::cout << regval << "\n"; } );
  YAML::Node read_config = helper.regAndVal2Yaml( reg_and_val_vec );
  YAML::Node toberemoved;
  for ( YAML::iterator it=read_config.begin(); it!=read_config.end(); ++it ){
    auto block_id = it->first.as<std::string>();
    for ( YAML::iterator jt=it->second.begin(); jt!=it->second.end(); ++jt ){
      auto subblock_id = jt->first.as<std::string>();
      for ( YAML::iterator kt=jt->second.begin(); kt!=jt->second.end(); ++kt ){
    	auto pname=kt->first.as<std::string>();
  	if( !config[block_id][subblock_id][pname].IsDefined() ){
	  spdlog::apply_all([&](LoggerPtr l) { l->trace("{}: Node {}.{}.{} was not found in config",m_name,block_id,subblock_id,pname); });
  	  toberemoved[block_id][subblock_id][pname]=0;
	}
      }
    }
  }
  for ( YAML::iterator it=toberemoved.begin(); it!=toberemoved.end(); ++it ){
    auto block_id = it->first.as<std::string>();
    for ( YAML::iterator jt=it->second.begin(); jt!=it->second.end(); ++jt ){
      auto subblock_id = jt->first.as<std::string>();
      for ( YAML::iterator kt=jt->second.begin(); kt!=jt->second.end(); ++kt ){
    	auto pname=kt->first.as<std::string>();
	spdlog::apply_all([&](LoggerPtr l) { l->trace("{}: Node {}.{}.{} will be removed",m_name,block_id,subblock_id,pname); });
  	read_config[block_id][subblock_id].remove( pname );
      }
    }
  }
  return read_config;
}

const std::vector<reg_and_val> Roc::i2c_transactions_from_yaml_config(const YAML::Node& config, bool readFlag)
{
  auto helper = HGCROC::roc_helper();
  auto str_and_val_vec = yaml_helper::flattenConfig(config);
  std::vector<reg_and_val> reg_and_val_vec;
  std::vector<reg_and_val>::iterator found_iter;

  std::for_each(str_and_val_vec.begin(),str_and_val_vec.end(),[&](const auto& sv){
    auto pname = sv.mStr;
    auto val = sv.mVal;
    for( auto reg : helper.getRegs(pname) ){	  
      if( !helper.isValidReg(reg) ) continue;
      auto r0=reg.R0;
      auto r1=reg.R1;
      auto a_reg_and_val = reg_and_val(r0,r1,val);
      unsigned int prev_val=0;
      if( (found_iter = std::find(reg_and_val_vec.begin(),reg_and_val_vec.end(),a_reg_and_val))!=reg_and_val_vec.end() ){
	spdlog::apply_all([&](LoggerPtr l) { l->trace("(R0, R1) = ( {} {} ) was already found ",r0,r1); });
	a_reg_and_val = (*found_iter);
	prev_val = a_reg_and_val.val();
	updateVal(a_reg_and_val,reg,prev_val,val);
	(*found_iter) = a_reg_and_val;
      }
      else{
	auto address = (r1<<8)|r0;
	prev_val = mCache.get(address);
	updateVal(a_reg_and_val,reg,prev_val,val);
	if( a_reg_and_val.val()!=prev_val || readFlag==true )
	  reg_and_val_vec.push_back( a_reg_and_val );
      }
      spdlog::apply_all([&](LoggerPtr l) { l->trace("{} {} {} {}",pname,r0,r1,a_reg_and_val.val()); });
    }
  });
  if( reg_and_val_vec.size() )
    std::sort( reg_and_val_vec.rbegin(), reg_and_val_vec.rend() );
  // std::for_each( reg_and_val_vec.begin(),
  // 		 reg_and_val_vec.end(),
  // 		 [](const auto &reg) {
  // 		   spdlog::apply_all([&](LoggerPtr l) { l->debug("reg address, r0, r1, val = ({} {} {} {})",reg.address(), reg.r0(), reg.r1(), reg.val()); } );
  // 		 } );
  return reg_and_val_vec;
}


const std::vector<reg_and_vals> Roc::optimized_i2c_transactions(const std::vector<reg_and_val>& reg_and_val_vec)
{
  std::vector<reg_and_vals> consec_reg_vec(1,reg_and_vals(*reg_and_val_vec.begin()));
  std::for_each( reg_and_val_vec.begin()+1,
		 reg_and_val_vec.end(),
		 [&](const auto& reg) {
		   auto consecutive_reg = std::find_if( consec_reg_vec.begin(), consec_reg_vec.end(), [&](const auto &consec_reg){return consec_reg.address()-reg.address()==1;} );
		   if( consecutive_reg==consec_reg_vec.end() ){
		     consec_reg_vec.push_back( reg_and_vals(reg) );
		   }
		   else
		     (*consecutive_reg).update(reg);
		 } );  
  return consec_reg_vec;
}


void Roc::updateCache( const std::vector<reg_and_val>& reg_and_val_vec )
{
  std::for_each( reg_and_val_vec.cbegin(),
		 reg_and_val_vec.cend(),
		 [&](const auto& a_reg_and_val){
		   mCache.set(a_reg_and_val.address(), a_reg_and_val.val());
		 });
}

void Roc::updateCache( const std::vector<reg_and_vals>& consec_reg_vec )
{
  std::for_each( consec_reg_vec.cbegin(),
  		 consec_reg_vec.cend(),
  		 [&](auto &consec_reg){
		   auto address=consec_reg.address();
		   auto reg_vals=consec_reg.vals();
		   for( auto val : reg_vals ){
		     mCache.set(address, val);
		     address++;
		   }
		 });
}

void Roc::writeI2C(std::vector<reg_and_vals>& consec_reg_vec)
{
  m_prev_addr = std::pair<unsigned int, unsigned int>(256,256);
  unsigned int reg_address_width=1;
  std::for_each( consec_reg_vec.begin(),
  		 consec_reg_vec.end(),
  		 [&](auto &consec_reg){
		   auto r0=consec_reg.r0();
		   auto r1=consec_reg.r1();
		   auto reg_vals=consec_reg.vals();
		   if( m_prev_addr.first!=r0 ){
		     m_transport->write_reg(m_address,r0);
		     m_prev_addr.first = r0;
		   }
		   if( m_prev_addr.second!=r1 ){
		     m_transport->write_reg(m_address+1,r1);
		     m_prev_addr.second = r1;
		   }
		   if( reg_vals.size()>1 ){
		     auto reg_address=reg_vals[0];
		     std::vector<unsigned int> data(reg_vals.begin()+1, reg_vals.end());
		     m_transport->write_regs(m_address+3,reg_address_width,reg_address,data);
		     m_prev_addr.first += reg_vals.size();
		   }
		   else{
		     m_transport->write_reg(m_address+2,reg_vals[0]); 
		   }
  		 } );
}

std::vector<reg_and_val> Roc::readI2C(std::vector<reg_and_vals>& consec_reg_vec)
{
  m_prev_addr = std::pair<unsigned int, unsigned int>(256,256);
  std::vector<reg_and_val> reg_and_val_vec;
  std::for_each( consec_reg_vec.begin(),
  		 consec_reg_vec.end(),
  		 [&](auto &consec_reg){
		   auto r0=consec_reg.r0();
		   auto r1=consec_reg.r1();
		   auto reg_vals=consec_reg.vals();
		   if( m_prev_addr.first!=r0 ){
		     m_transport->write_reg(m_address,r0);
		     m_prev_addr.first = r0;
		   }
		   if( m_prev_addr.second!=r1 ){
		     m_transport->write_reg(m_address+1,r1);
		     m_prev_addr.second = r1;
		   }
		   if( reg_vals.size()>1 ){
		    // auto read_vals = m_transport->read_regs(m_address+3,reg_vals.size());
		    // std::transform( read_vals.begin(), read_vals.end(),
		    // 		     std::back_inserter(reg_and_val_vec),
		    // 		     [&r0,&r1](const auto& val){
		    // 		       reg_and_val rval(r0,r1,val);
		    // 		       r0++;
		    // 		       return rval;
		    // 		     });
		     unsigned int n=0;
		     std::for_each(reg_vals.begin(),
				   reg_vals.end(),
				   [&]([[maybe_unused]] const auto& val){
				     auto rval = m_transport->read_reg(m_address+3);
				     reg_and_val_vec.push_back( reg_and_val(r0+n,r1,rval) );
				     n++;
				   });
		     m_prev_addr.first += reg_vals.size();
		   }
		   else{
 		     auto val = m_transport->read_reg(m_address+2);
		     reg_and_val_vec.push_back( reg_and_val(r0,r1,val) );
		   }
  		 } );
  return reg_and_val_vec;
}

void Roc::writeDirectAccessI2C( std::map<unsigned int,unsigned int>& addrAndValMap )
{
  std::for_each( addrAndValMap.begin(),
  		 addrAndValMap.end(),
  		 [&](auto &addrAndVal){
		   auto slave_address = m_address + addrAndVal.first;
		   auto val = addrAndVal.second;
		   m_transport->write_reg(slave_address,val);
  		 } );
}

std::map<unsigned int,unsigned int> Roc::readDirectAccessI2C( std::map<unsigned int,unsigned int>& addrAndValMap )
{
  std::map<unsigned int,unsigned int> retMap;
  std::for_each( addrAndValMap.begin(),
  		 addrAndValMap.end(),
  		 [&](auto &addrAndVal){
		   auto slave_address = m_address + addrAndVal.first;
		   auto r_val = m_transport->read_reg(slave_address);
		   retMap[addrAndVal.first] = r_val;
  		 } );
  return retMap;
}

