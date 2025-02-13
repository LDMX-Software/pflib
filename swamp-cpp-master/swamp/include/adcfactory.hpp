#pragma once

#include <boost/function.hpp>
#include <boost/functional/factory.hpp>
#include <boost/bind/bind.hpp>
#include <memory>

#include "adc.hpp"
#include "lpgbt_adc.hpp"

using namespace boost::placeholders;


class ADCFactory
{
public:
  ADCFactory(){
    adcmap["lpgbt_adc"]   = boost::bind( boost::factory<LPGBT_ADC*>(), _1, _2 );
  }
  ~ADCFactory(){;}
  
  ADCPtr Create(const std::string& key, std::string aname, YAML::Node node ) const
  {
    if( adcmap.find(key)==adcmap.end() ){
      auto errorMsg = "Trying to create adc {} of type {} which is not implemented in chip factory";
      spdlog::apply_all([&](LoggerPtr l) { l->error( errorMsg,aname,key); });
      throw swamp::UnknownADCException(errorMsg);
    }
    ADCPtr ptr{adcmap.at(key)(aname,node)};
    return ptr;
  }

private:
  std::map<std::string, boost::function<ADC* (const std::string&, const YAML::Node&)>> adcmap;
};

