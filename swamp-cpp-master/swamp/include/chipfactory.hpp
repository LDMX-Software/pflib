#pragma once

#include <boost/function.hpp>
#include <boost/functional/factory.hpp>
#include <boost/bind/bind.hpp>
#include <memory>

#include "chip.hpp"
#include "roc.hpp"
#include "econ.hpp"
#include "vtrxp.hpp"
#include "lpgbt.hpp"
#include "dummyChip.hpp"

using namespace boost::placeholders;

class ChipFactory
{
 public:
  ChipFactory(){
    chipmap["roc"]   = boost::bind( boost::factory<Roc*>()  , _1, _2 );
    chipmap["econ"]  = boost::bind( boost::factory<Econ*>() , _1, _2 );
    chipmap["vtrxp"] = boost::bind( boost::factory<VTRXp*>(), _1, _2 );
    chipmap["lpgbt"] = boost::bind( boost::factory<Lpgbt*>(), _1, _2 );
    chipmap["dummy"] = boost::bind( boost::factory<DummyChip*>(), _1, _2 );
  }
  ~ChipFactory(){;}
  
  ChipPtr Create(const std::string& key, std::string aname, YAML::Node node ) const
  {
    if( chipmap.find(key)==chipmap.end() ){
      auto errorMsg = "Trying to create chip {} which is not implemented in chip factory";
      spdlog::apply_all([&](LoggerPtr l) { l->error( errorMsg,key); });
      throw swamp::UnknownChipException(errorMsg);
    }
    ChipPtr ptr{chipmap.at(key)(aname,node)};
    return ptr;
  }

 private:
  std::map<std::string, boost::function<Chip* (const std::string&, const YAML::Node&)>> chipmap;
};

namespace swamp
{
  static bool IsChipOfType(ChipPtr chip, ChipType ctype)
  {
    return (chip->chipType() & ctype) == ctype;
  } 
}
