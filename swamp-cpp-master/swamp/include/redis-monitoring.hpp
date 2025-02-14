#pragma once

#include <sw/redis++/redis++.h>
#include <iostream>

class RedisMonitoring
{
public:

  RedisMonitoring() : mRedis( sw::redis::Redis("tcp://127.0.0.1:6379") )
  {
    
  }
  
  RedisMonitoring( std::string key, std::unordered_map< std::string, uint32_t > counterMap ) : mKey(key),
											       mRedis( sw::redis::Redis("tcp://127.0.0.1:6379") )
  {
    auto exists = mRedis.exists(mKey);
    if (!exists)
      mRedis.hmset(mKey,counterMap.begin(),counterMap.end());
  }

  uint32_t getCounter( std::string counterField )
  {
    auto count = mRedis.hget(mKey,counterField);
    return std::stol(count.value());
  }

  void incrementCounter( std::string counterField, uint32_t increment=1 )
  {
    auto count = getCounter(counterField);
    mRedis.hset(mKey,counterField,std::to_string(count+increment));
  }
protected:
  std::string mKey;
  sw::redis::Redis mRedis;
  
};
