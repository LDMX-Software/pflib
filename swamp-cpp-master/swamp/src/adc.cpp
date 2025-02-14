#include "adc.hpp"

ADC::ADC( const std::string& name, const YAML::Node& config) : m_name(name)
{
  std::ostringstream os( std::ostringstream::ate );
  os.str(""); os << config ;
  spdlog::apply_all([&](LoggerPtr l) { l->debug("Creating ADC {} with config \n{}",m_name,os.str().c_str()); });

  m_measurementBuffer = boost::circular_buffer<ADCMeasurementOutputPtr>(MEASUREMENT_BUFFER_SIZE);
}

void ADC::configure(const YAML::Node& node)
{
  std::ostringstream os( std::ostringstream::ate );
  os.str(""); os << node ;
  spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : configuring with config \n{}",m_name,os.str().c_str()); });
  lock();
  ConfigureImpl(node);
  unlock();
}

void ADC::acquire(unsigned int nSamples)
{
  spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : acquiring ADC measurements with {} samples for each registered measurement",m_name,nSamples); });
  lock();
  AcquireImpl(nSamples);
  unlock();
}
