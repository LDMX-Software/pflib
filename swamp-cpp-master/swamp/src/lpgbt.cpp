#include "lpgbt.hpp"
#include <stdio.h>
#include "lpgbt_helper.hpp"
#include <chrono>

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::milliseconds ms;
typedef std::chrono::duration<float> fsec;

Lpgbt::Lpgbt( const std::string& name, const YAML::Node& config) : Chip(name,config),
								   mCache( AsicCache(name) )
{
  if( !mCache.existsInCache() )
    buildCache();
  else{
    spdlog::apply_all([&](LoggerPtr l) { l->debug( "{} : cache of already exists",m_name ); });
  }
}

void Lpgbt::buildCache()
{
  std::string templateName("lpgbt_template");
  auto cacheCopy = mCache.copyFromTemplate( templateName );
  if( !cacheCopy ){
    spdlog::apply_all([&](LoggerPtr l) { l->debug( "{} : building cache from default register map",m_name ); });
    auto helper = LPGBT::lpgbt_helper();
    mCache.set(helper.defaultRegMap());
  }
  else
    spdlog::apply_all([&](LoggerPtr l) { l->debug( "{} : building cache from template {}",m_name, templateName); });
}

std::vector<lpgbt_reg_and_val> Lpgbt::getFullCache()
{
  spdlog::apply_all([&](LoggerPtr l) { l->debug( "{} : get full cache",m_name ); });
  auto helper = LPGBT::lpgbt_helper();
  auto addresses = helper.reg_addresses();
  auto cacheMap = mCache.get(addresses);
  std::vector<lpgbt_reg_and_val> reg_and_val_vec; reg_and_val_vec.reserve(cacheMap.size());
  std::transform(cacheMap.cbegin(), cacheMap.cend(),
		 std::back_inserter(reg_and_val_vec),
		 [&](const auto& p){
		   return lpgbt_reg_and_val(p.first,p.second);
		 });
  return reg_and_val_vec;
}

void Lpgbt::ConfigureImpl( const YAML::Node& config )
{
  auto reg_vec = yaml_2_reg_and_val(config);
  auto consec_reg_vec = reg_and_val_2_consecutive_reg_and_vals(reg_vec);
  const auto start{Time::now()};
  write_regs(consec_reg_vec);
  const auto end{Time::now()};
  fsec elapsed_seconds{end - start};
  spdlog::apply_all([&](LoggerPtr l) { l->info( "{} : time to write_regs = {} ms",m_name,elapsed_seconds.count() ); });
}

const YAML::Node Lpgbt::ReadImpl( const YAML::Node& config, bool fromHardware )
{
  std::vector<lpgbt_reg_and_val> reg_and_val_vec;
  auto reg_vec = yaml_2_reg_and_val(config);
  if( fromHardware ){
    auto consec_reg_vec = reg_and_val_2_consecutive_reg_and_vals(reg_vec);
    reg_and_val_vec = read_regs(consec_reg_vec);
  }
  else{
    for( auto reg_val : reg_vec ){
      auto address = reg_val.reg_address();
      auto val = mCache.get(address);
      reg_and_val_vec.push_back( lpgbt_reg_and_val(address,val) );
    }
  }

  // spdlog::apply_all([&](LoggerPtr l) { l->debug("{} read: size of reg_and_val_vec = {}", m_name, reg_and_val_vec.size() ); });  
  // std::for_each( reg_and_val_vec.cbegin(), reg_and_val_vec.cend(),
  // 		 [&](const auto& rv){
  // 		   spdlog::apply_all([&](LoggerPtr l) { l->debug("{} read: reg_and_val (address,val) = (0x{:02x},0x{:02x})", m_name, rv.reg_address(), rv.val() ); });
  // 		 }) ;
  auto helper = LPGBT::lpgbt_helper();
  auto read_config = helper.reg_and_val_2_yaml( reg_and_val_vec );
  return read_config;
}

const YAML::Node Lpgbt::ReadImpl( bool fromHardware ) 
{
  std::vector<lpgbt_reg_and_val> reg_and_val_vec;
  std::vector<lpgbt_reg_and_val> reg_and_val_vec_from_cache = getFullCache();
  reg_and_val_vec.reserve(reg_and_val_vec_from_cache.size());
  if( fromHardware ){
    auto consec_reg_vec = reg_and_val_2_consecutive_reg_and_vals(reg_and_val_vec_from_cache);
    reg_and_val_vec = read_regs(consec_reg_vec);
  }
  else
    reg_and_val_vec = reg_and_val_vec_from_cache;

  std::for_each( reg_and_val_vec.cbegin(), reg_and_val_vec.cend(),
		 [](const auto& v){
		   spdlog::apply_all([&](LoggerPtr l) { l->trace("Registers in reg_and_val_vec after reading (address,val) = (0x{:02x},0x{:02x})", v.reg_address(), v.val() ); });
		 }) ;
  
  auto helper = LPGBT::lpgbt_helper();
  return helper.reg_and_val_2_yaml(reg_and_val_vec);
}  

const std::vector<lpgbt_reg_and_val> Lpgbt::yaml_2_reg_and_val(const YAML::Node& config, bool readFlag)
{
  auto helper = LPGBT::lpgbt_helper();
  std::vector<lpgbt_reg_and_val> vec;
  vec.reserve(config.size());
  for( auto reg_and_val: config ){
    auto regname = reg_and_val.first.as< std::string >();
    auto val = reg_and_val.second.as< uint16_t >();
    if( helper.checkIfRegIsInRegMap(regname) ){
      auto reg = helper.getRegister(regname);
      auto a_reg_and_val = lpgbt_reg_and_val( reg.address, val );
      vec.push_back( a_reg_and_val );
    }
    else{
      spdlog::apply_all([&](LoggerPtr l) { l->error("Register {} was not found in register map",regname); });
    }
  }
  std::sort( vec.rbegin(), vec.rend() );
  std::for_each( vec.cbegin(), vec.cend(),
		 [](const auto& v){
		   spdlog::apply_all([&](LoggerPtr l) { l->trace("Registers in sorted reg_and_val (address,val) = (0x{:02x},0x{:02x})", v.reg_address(), v.val() ); });
		 }) ;
  return vec;
}

const std::vector<lpgbt_reg_and_vals> Lpgbt::reg_and_val_2_consecutive_reg_and_vals(const std::vector<lpgbt_reg_and_val>& reg_and_val_vec)
{
  std::vector<lpgbt_reg_and_vals> consec_reg_vec(1,lpgbt_reg_and_vals(*reg_and_val_vec.begin()));
  std::for_each( reg_and_val_vec.cbegin()+1,
  		 reg_and_val_vec.cend(),
  		 [&](const auto& reg) {
  		   auto consecutive_reg = std::find_if( consec_reg_vec.begin(), consec_reg_vec.end(), [&](const auto &consec_reg){return consec_reg.reg_address()-reg.reg_address()==1;} );
  		   if( consecutive_reg==consec_reg_vec.end() ){
  		     consec_reg_vec.push_back( lpgbt_reg_and_vals(reg) );
  		   }
  		   else
  		     (*consecutive_reg).update(reg);
  		 } );  
  std::for_each( consec_reg_vec.cbegin(), consec_reg_vec.cend(),
  		 [](const auto& v){
  		   std::ostringstream os( std::ostringstream::ate );
  		   os.str("");
  		   os << v;
  		   spdlog::apply_all([&](LoggerPtr l) { l->trace("Creating lpgbt transaction with (reg_address, vals): {}", os.str().c_str()); });
  		 }) ;
  return consec_reg_vec;
}

void Lpgbt::write_regs(std::vector<lpgbt_reg_and_vals>& consec_reg_vec)
{
  std::vector<unsigned int> reg_addr_vec;
  reg_addr_vec.reserve(consec_reg_vec.size());
  std::vector< std::vector<unsigned int> > reg_vals_vec;
  std::for_each( consec_reg_vec.begin(), consec_reg_vec.end(), [&reg_addr_vec,&reg_vals_vec](const auto& consec_reg){
    reg_addr_vec.push_back(consec_reg.reg_address());
    reg_vals_vec.push_back(consec_reg.vals());
  });
  		   
  for(unsigned int i(0); i<reg_addr_vec.size(); i++)
      spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : regs,size : {},{}", m_name,reg_addr_vec[i],reg_vals_vec[i].size()); });
  
  m_transport->write_regs(m_address,LPGBT_REG_ADDRESS_WIDTH,reg_addr_vec,reg_vals_vec); 
  updateCache(consec_reg_vec);
}

std::vector<lpgbt_reg_and_val> Lpgbt::read_regs(std::vector<lpgbt_reg_and_vals>& consec_reg_vec)
{
  std::vector<lpgbt_reg_and_val> reg_and_val_vec;
  std::for_each( consec_reg_vec.begin(),
  		 consec_reg_vec.end(),
  		 [&](auto &consec_reg){
		   auto read_len = consec_reg.vals().size();
  		   auto vec = m_transport->read_regs(m_address,LPGBT_REG_ADDRESS_WIDTH,consec_reg.reg_address(),read_len);
		   auto reg_addr = consec_reg.reg_address();
		   std::for_each( vec.cbegin(), vec.cend(),
				  [&](const auto& v){
				    lpgbt_reg_and_val reg_and_val(reg_addr,v);
				    reg_and_val_vec.push_back(reg_and_val);
				    reg_addr++;
				  } );
  		 } );
  updateCache(reg_and_val_vec);
  return reg_and_val_vec;
}

void Lpgbt::write_regs( unsigned int reg_addr, const std::vector<unsigned int>& reg_vals )
{
  m_transport->write_regs(m_address,LPGBT_REG_ADDRESS_WIDTH,reg_addr,reg_vals); 
  updateCache(reg_addr,reg_vals);
}

void Lpgbt::write_regs(const std::vector<unsigned int>& reg_addr_vec, const std::vector<std::vector<unsigned int>>& reg_vals_vec)
{
  m_transport->write_regs(m_address,LPGBT_REG_ADDRESS_WIDTH,reg_addr_vec,reg_vals_vec);
  for(unsigned int i(0); i<reg_addr_vec.size(); i++)
    updateCache(reg_addr_vec[i],reg_vals_vec[i]);
}


void Lpgbt::write_reg( unsigned int reg_addr, unsigned int reg_val, unsigned int mask )
{
  unsigned int a_reg_val = reg_val;
  if( mask!=0xFF ){
    auto prev_val = mCache.get(reg_addr);
    a_reg_val = (prev_val & (~mask)) | (reg_val << int(std::log2(mask&-mask)) ); //ie get least significant bit at 1
  }

  std::vector<unsigned int> reg_vals(1,a_reg_val);
  write_regs(reg_addr,reg_vals);
}
    
const std::vector<unsigned int> Lpgbt::read_regs( unsigned int reg_addr, unsigned int read_len )
{
  auto vals = m_transport->read_regs(m_address,LPGBT_REG_ADDRESS_WIDTH,reg_addr,read_len);
  updateCache(reg_addr,vals);
  return vals;
}

const std::vector<unsigned int> Lpgbt::read_regs( const std::vector<unsigned int>& reg_addr_vec, const std::vector<unsigned int>& read_len_vec )
{
  auto vals = m_transport->read_regs(m_address,LPGBT_REG_ADDRESS_WIDTH,reg_addr_vec,read_len_vec);
  size_t ptr(0);
  for(unsigned int i(0); i<reg_addr_vec.size(); i++){
    std::vector<unsigned int> vec(read_len_vec[i]);
    std::copy(vals.begin()+ptr, vals.begin()+ptr+read_len_vec[i], vec.begin());
    ptr+=read_len_vec[i];
    updateCache(reg_addr_vec[i],vec);
  }
  //updateCache(reg_addr,vals);
  return vals;
}

const unsigned int Lpgbt::read_reg( unsigned int reg_addr )
{
  return read_regs( reg_addr, 0x1 )[0];
}

void Lpgbt::updateCache( const std::vector<lpgbt_reg_and_val>& reg_and_val_vec )
{
  std::map<uint16_t,uint8_t> aMap;
  std::for_each(reg_and_val_vec.cbegin(),
		reg_and_val_vec.cend(),
		[&aMap](const auto& reg_val){
		  auto address = reg_val.reg_address();
		  auto val = reg_val.val();
		  aMap[ address ] = (uint8_t)val;
		});
  mCache.set(aMap);
}

void Lpgbt::updateCache( const std::vector<lpgbt_reg_and_vals>& consec_reg_vec )
{
  std::map<uint16_t,uint8_t> aMap;
  std::for_each( consec_reg_vec.begin(),
  		 consec_reg_vec.end(),
  		 [&](auto &consec_reg){
		   uint16_t regid = 0;
		   uint16_t address = consec_reg.reg_address();
		   auto vals = consec_reg.vals();
		   for( auto val : vals ){
		     aMap[address+regid] = (uint8_t)val;
		     regid++;
		   }
		 });
  mCache.set(aMap);
}

void Lpgbt::updateCache( const uint16_t reg_address, const std::vector<unsigned int>& vals )
{
  unsigned int regid=0;
  std::for_each(vals.begin(),
		vals.end(),
		[&](const auto& val){
		  mCache.set(reg_address+regid,(uint8_t)val);
		  regid++;
		});
}
