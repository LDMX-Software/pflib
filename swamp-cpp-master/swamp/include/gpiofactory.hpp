#pragma once

#include <boost/function.hpp>
#include <boost/functional/factory.hpp>
#include <boost/bind/bind.hpp>
#include <memory>

#include "gpio.hpp"
#include "lpgbt_gpio.hpp"

using namespace boost::placeholders;

class GPIOFactory
{
 public:
  GPIOFactory(){
    gpioMap["lpgbt_gpio"]     = boost::bind( boost::factory<LPGBT_GPIO*>(), _1, _2 );
  }
  ~GPIOFactory(){;}
  
  GPIOPtr Create(const std::string& key, std::string aname, YAML::Node node ) const
  {
    if( gpioMap.find(key)==gpioMap.end() ){
      auto errorMsg = "Trying to create gpio {} which is not implemented in gpiofactory";
      spdlog::apply_all([&](LoggerPtr l) { l->error( errorMsg,key); });
      throw swamp::UnknownGPIOException(errorMsg);
    }
    GPIOPtr ptr{gpioMap.at(key)(aname,node)};
    return ptr;
  }

 private:
  std::map<std::string, boost::function<GPIO* (const std::string&, const YAML::Node&)>> gpioMap;
};

namespace swamp
{
  static bool IsGPIOOfType(GPIOPtr gpio, GPIOType gtype)
  {
    return (gpio->gpioType() & gtype) == gtype;
  } 
}
