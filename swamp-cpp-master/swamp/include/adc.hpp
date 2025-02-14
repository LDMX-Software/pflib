#pragma once
#include "carrier.hpp"
#include <boost/circular_buffer.hpp>
#include <boost/thread/thread.hpp>

#define MEASUREMENT_BUFFER_SIZE 20

struct ADCMeasurementOutput
{
  ADCMeasurementOutput( std::string aName ) : name(aName) {;}
  ADCMeasurementOutput( std::string aName, std::vector<unsigned int> aRawValues ) : name(aName),
										    rawValues(aRawValues)
  {;}
  ADCMeasurementOutput( std::string aName, std::vector<unsigned int> aRawValues, std::vector<float> aValues ) : name(aName),
														rawValues(aRawValues),
														values(aValues)
 {;}
  std::string name;
  std::vector<float> values;
  std::vector<unsigned int> rawValues;
};

using ADCMeasurementOutputPtr = std::shared_ptr<ADCMeasurementOutput>;


class ADC
{
public:

  ADC( const std::string& name, const YAML::Node& config);

  virtual ~ADC() = default;

  inline void setCarrier( CarrierPtr carrier ){ m_carrier = carrier; }

  inline void setLogger( LoggerPtr logger )
  {
    m_logger=logger;
  }

  inline void lock()
  {
    spdlog::apply_all([&](LoggerPtr l) { l->trace("{} : trying to lock resource",m_name); });
    m_carrier->lock();
  }

  inline void unlock()
  {
    spdlog::apply_all([&](LoggerPtr l) { l->trace("{} : unlocking resource",m_name); });
    m_carrier->unlock();
  }
  
  void configure(const YAML::Node& node);

  void acquire(unsigned int nSamples=1);

  // void acquire_forever();
  
  inline ADCMeasurementOutputPtr getLatestADCMeasurement()
  {
    if( m_measurementBuffer.size() ){
      boost::lock_guard<boost::mutex> lock{m_mutex};
      auto adcMeasurement = m_measurementBuffer[0];
      m_measurementBuffer.pop_front();
      return adcMeasurement;
    }
    return std::make_shared<ADCMeasurementOutput>("null");
  }

  inline void fillADCMeasurementBuffer(ADCMeasurementOutputPtr adcMeasurement)
  {
    boost::lock_guard<boost::mutex> lock{m_mutex};
    m_measurementBuffer.push_back(adcMeasurement);
  }
  
protected:
  virtual void ConfigureImpl(const YAML::Node& node) = 0;
  
  virtual void AcquireImpl(unsigned int nSamples=1) = 0;

protected:
  std::string m_name;
  
  CarrierPtr m_carrier;

  LoggerPtr m_logger;

  boost::circular_buffer<ADCMeasurementOutputPtr> m_measurementBuffer;

  boost::mutex m_mutex;
};

using ADCPtr = std::shared_ptr<ADC>;
 
