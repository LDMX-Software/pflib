#pragma once
#include "unordered_map"

#include "carrier.hpp"
#include "redis-asic-cache.hpp"
#include "swamp_type.hpp"

enum GPIO_Direction { INPUT,
		      OUTPUT };

static std::unordered_map< std::string, GPIO_Direction> GPIODirectionMap = {
									    {"input", GPIO_Direction::INPUT},
									    {"output", GPIO_Direction::OUTPUT}
};

class GPIO
{
public:

  GPIO( const std::string& name, const YAML::Node& config);

  virtual ~GPIO() = default;

  inline void setCarrier( CarrierPtr carrier ){ m_carrier = carrier; }

  inline void setLogger( LoggerPtr logger )
  {
    m_logger=logger;
  }

  inline void lock()
  {
    spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : trying to lock ressource",m_name); });
    m_carrier->lock();
  }

  inline void unlock()
  {
    spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : unlocking ressource",m_name); });
    m_carrier->unlock();
  }

  void up();

  void down();

  bool is_up();

  bool is_down();

  void set_direction();

  void set_direction(GPIO_Direction dir);

  GPIO_Direction get_direction();

  void configure(const YAML::Node& node);

  void resetAsicCache();
  
  const swamp::GPIOType gpioType()
  {
    return m_gpiotype;
  }

protected:
  virtual void UpImpl() = 0;

  virtual void DownImpl() = 0;

  virtual bool IsUpImpl() = 0;

  virtual bool IsDownImpl() = 0;

  virtual void ConfigureImpl(const YAML::Node& node) = 0;

  virtual void SetDirectionImpl(GPIO_Direction dir) = 0;

  virtual GPIO_Direction GetDirectionImpl() = 0;
protected:

  std::string m_name;
  
  CarrierPtr m_carrier;

  LoggerPtr m_logger;

  unsigned int m_pin;

  GPIO_Direction m_direction;

  std::map<std::string,std::string> m_chip2Reset;

  swamp::GPIOType m_gpiotype;
};

using GPIOPtr = std::shared_ptr<GPIO>;
 
