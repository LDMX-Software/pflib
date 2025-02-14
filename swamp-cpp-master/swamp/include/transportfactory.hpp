#pragma once

#include <boost/function.hpp>
#include <boost/functional/factory.hpp>
#include <boost/bind/bind.hpp>
#include <memory>

#include "transport.hpp"
#include "dummyTransport.hpp"
#include "ic_ec_with_emp.hpp"
#include "ic_ec_with_sct.hpp"
#include "lpgbt_i2c.hpp"

using namespace boost::placeholders;

class TransportFactory
{
 public:
  TransportFactory(){
    transportMap["dummy"]          = boost::bind( boost::factory<DummyTransport*>(), _1, _2 );
    transportMap["ic_ec_with_emp"] = boost::bind( boost::factory<ICECWithEMP*>(), _1, _2 );
    transportMap["ic_ec_with_sct"] = boost::bind( boost::factory<ICECWithSCT*>(), _1, _2 );
    transportMap["lpgbt_i2c"]      = boost::bind( boost::factory<LPGBT_I2C*>(), _1, _2 );
  }
  ~TransportFactory(){;}
  
  TransportPtr Create(const std::string& key, std::string aname, YAML::Node node ) const
  {
    if( transportMap.find(key)==transportMap.end() ){
      auto errorMsg = "Trying to create transport {} which is not implemented in transportfactory";
      spdlog::apply_all([&](LoggerPtr l) { l->error( errorMsg,key); });
      throw swamp::UnknownTransportException(errorMsg);
    }
    TransportPtr ptr{transportMap.at(key)(aname,node)};
    return ptr;
  }

 private:
  std::map<std::string, boost::function<Transport* (const std::string&, const YAML::Node&)>> transportMap;
};

namespace swamp
{
  static bool IsTransportOfType(TransportPtr transport, TransportType ttype)
  {
    return (transport->transportType() & ttype) == ttype;
  } 
}
