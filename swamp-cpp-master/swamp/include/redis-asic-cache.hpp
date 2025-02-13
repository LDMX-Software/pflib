#pragma once
#include <sw/redis++/redis++.h>
#include <iostream>

class AsicCache
{
public:
  AsicCache(std::string asicName) : mAsicName(asicName),
				    mRedis( sw::redis::Redis("tcp://127.0.0.1:6379") )
  {
    mExists = mRedis.exists(mAsicName);
  }
  
  bool existsInCache(){ return mExists; }

  bool copyFromTemplate(std::string templateName)
  {
    auto templateExists = mRedis.exists(templateName);
    if(templateExists){
      std::vector<std::string> ops = {"copy", templateName, mAsicName};
      mRedis.command(ops.begin(), ops.end());
      mExists=true;
      return true;
    }
    return false;
  }

  void set(const std::map<uint16_t,uint8_t>& regmap)
  {
    std::vector<std::string> ops = {"bitfield", mAsicName};
    std::for_each(regmap.cbegin(),regmap.cend(),
                  [&ops](const auto& p){
                    std::vector<std::string> vec{"SET", "u8", "#"+std::to_string(p.first), std::to_string(p.second)};
                    ops.insert(ops.end(),vec.begin(),vec.end());
                  });
                  
    mRedis.command<std::optional<std::vector<std::optional<long long>>>>(ops.begin(), ops.end());
  }

  void set(uint16_t regAddress, uint8_t val)
  {
    std::vector<std::string> ops = {"bitfield", mAsicName, "SET", "u8", "#"+std::to_string(regAddress), std::to_string(val)};
    mRedis.command<std::optional<std::vector<std::optional<long long>>>>(ops.begin(), ops.end());
  }
  
  uint8_t get(uint16_t regAddress)
  {
    std::vector<std::string> ops = {"bitfield", mAsicName, "GET", "u8", "#"+std::to_string(regAddress) };
    auto val = mRedis.command<std::optional<std::vector<std::optional<long long>>>>(ops.begin(), ops.end());
    return val.value()[0].value();
  }

  std::map<uint16_t,uint8_t> get(std::set<uint16_t> regAddresses)
  {
    std::vector<std::string> ops = {"bitfield", mAsicName};
    std::for_each(regAddresses.cbegin(),regAddresses.cend(),
		  [&ops](const auto& addr){
		    std::vector<std::string> vec{"GET", "u8", "#"+std::to_string(addr)};
		    ops.insert(ops.end(),vec.begin(),vec.end());
		  });
    auto optvec = mRedis.command<std::optional<std::vector<std::optional<long long>>>>(ops.begin(), ops.end());

    std::map<uint16_t,uint8_t> amap;
    std::transform( regAddresses.begin(), regAddresses.end(),
		    optvec.value().begin(), std::inserter(amap,amap.end()),
		    [](uint16_t addr, const auto& optval)
		    {
		      return std::make_pair(addr,optval.value());
		    } );
    return amap;
  }

  void deleteCache()
  {
    mRedis.del(mAsicName);
  }
  
protected:
  bool mExists;
  std::string mAsicName;
  sw::redis::Redis mRedis;
};


struct CacheReseter{
  static void ResetCache(std::string asicName, std::string templateName){ 
    auto redis=sw::redis::Redis( sw::redis::Redis("tcp://127.0.0.1:6379") );
    std::vector<std::string> ops = {"copy", templateName, asicName,"replace"};
    redis.command(ops.begin(), ops.end());
  }
};
