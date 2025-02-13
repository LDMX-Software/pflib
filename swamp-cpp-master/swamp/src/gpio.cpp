#include "gpio.hpp"

GPIO::GPIO( const std::string& name, const YAML::Node& config) : m_name(name)
{
  std::ostringstream os( std::ostringstream::ate );
  os.str(""); os << config ;
  spdlog::apply_all([&](LoggerPtr l) { l->debug("Creating GPIO {} with config \n{}",m_name,os.str().c_str()); });
    
  if( config["pin"].IsDefined() )
    m_pin = config["pin"].as<unsigned int>();
  else{
    auto errorMsg = "'pin' key is missing in {} GPIO configuration";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name); });
    throw swamp::GPIOConstructionException(errorMsg);
  }

  if( config["direction"].IsDefined() ){
    std::string direction = config["direction"].as<std::string>();
    std::transform(direction.begin(),
		   direction.end(),
		   direction.begin(),
		   [](const char& c){ return std::tolower(c);} );
    auto it = GPIODirectionMap.find(direction);
    if (it != GPIODirectionMap.end())
      m_direction = it->second;
    else{
      auto errorMsg = "Direction of GPIO {} should be either \"input\" or \"output\"";
      spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name); });
      throw swamp::GPIOConstructionException(errorMsg);
    }
  }
  else{
    auto errorMsg = "'direction' key is missing in {} GPIO configuration";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,m_name); });
    throw swamp::GPIOConstructionException(errorMsg);
  }
    
  if( config["chip2Reset"].IsDefined() && config["chip2Reset"].IsSequence() ){
    std::ostringstream os(std::ostringstream::ate);
    os.str("");
    for( auto node: config["chip2Reset"] ){
      std::string name = node["name"].as<std::string>();
      std::string templateName = node["template"].as<std::string>();
      m_chip2Reset.insert( std::pair<std::string,std::string>(name, templateName) );
      os << name << " (reset with template : " << templateName << ")\n";
    }
    spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : is configured to reset the following list of chips when toggled down: \n{}",m_name, os.str().c_str()); });      
  }
  
  std::string gpioTypeStr = "GENERIC";
  if( config["gpioType"].IsDefined() )
    gpioTypeStr = config["gpioType"].as<std::string>();
  else{
    auto warnMsg = "gpioType was not found in gpio config (in constructor) -> generic type will be applied";
    spdlog::apply_all([&](LoggerPtr l) { l->warn(warnMsg); });
  }
  
  std::transform(gpioTypeStr.begin(), gpioTypeStr.end(), gpioTypeStr.begin(), 
		 [](const char& c){ return std::toupper(c);} );

  std::unordered_map<std::string,swamp::GPIOType> typeMap{ {"GENERIC"      ,swamp::GPIOType::GENERIC},
							   {"RSTB"         ,swamp::GPIOType::RSTB},
							   {"PWR_EN"       ,swamp::GPIOType::PWR_EN},
							   {"PWR_GD"       ,swamp::GPIOType::PWR_GD},
							   {"PWR_GD_DCDC"  ,swamp::GPIOType::PWR_GD_DCDC},
							   {"PWR_GD_LDO"   ,swamp::GPIOType::PWR_GD_LDO},
							   {"HGCROC_RE"    ,swamp::GPIOType::HGCROC_RE},
							   {"HGCROC_RE_SB" ,swamp::GPIOType::HGCROC_RE_SB},
							   {"HGCROC_RE_HB" ,swamp::GPIOType::HGCROC_RE_HB},
							   {"ECON_RE"      ,swamp::GPIOType::ECON_RE},
							   {"ECON_RE_SB"   ,swamp::GPIOType::ECON_RE_SB},
							   {"ECON_RE_HB"   ,swamp::GPIOType::ECON_RE_HB},
							   {"READY"        ,swamp::GPIOType::READY}
  };
    
  if( auto it = typeMap.find( gpioTypeStr ); it != typeMap.end() ){
    m_gpiotype = it->second;
  }
  else{
    auto errorMsg = "A gpiotype ({}) was not found in gpio type Map (in GPIO constructor)";
    spdlog::apply_all([&](LoggerPtr l) { l->critical(errorMsg,gpioTypeStr); });
    throw swamp::GPIOConstructionException(errorMsg);      
  }
}


void GPIO::up()
{
  spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : going up",m_name); });
  lock();
  UpImpl();
  unlock();
}

void GPIO::down()
{
  spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : going down",m_name); });
  lock();
  DownImpl();
  resetAsicCache();
  unlock();
}

bool GPIO::is_up()
{
  spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : checking if up",m_name); });
  lock();
  auto val = IsUpImpl();
  unlock();
  return val;
}

bool GPIO::is_down()
{
  spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : checking if down",m_name); });
  lock();
  auto val = IsDownImpl();
  unlock();
  return val;
}

void GPIO::configure(const YAML::Node& node)
{
  std::ostringstream os( std::ostringstream::ate );
  os.str(""); os << node ;
  spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : configuring with config \n{}",m_name,os.str().c_str()); });
  lock();
  ConfigureImpl(node);
  unlock();
}

void GPIO::set_direction()
{
  set_direction(m_direction);
}

void GPIO::set_direction(GPIO_Direction dir)
{
  spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : setting pin direction to {}",m_name,dir); });
  lock();
  SetDirectionImpl(dir);
  unlock();
}

GPIO_Direction GPIO::get_direction()
{
  spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : reading pin direction",m_name); });
  lock();
  auto dir = GetDirectionImpl();
  unlock();
  spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : pin direction = ",m_name,dir); });
  return dir;
}

void GPIO::resetAsicCache()
{
  std::for_each( m_chip2Reset.begin(), m_chip2Reset.end(),
		 [&](auto &p){
		   spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : reseting cache of {}, with template {}",m_name, p.first, p.second); });
		   CacheReseter::ResetCache(p.first,p.second);
		 });
}
