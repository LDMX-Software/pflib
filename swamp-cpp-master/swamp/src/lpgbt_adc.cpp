#include "lpgbt_adc.hpp"
#include "timer.hpp"

LPGBT_ADC::LPGBT_ADC( const std::string& name, const YAML::Node& config) : ADC(name,config)
{ 
  if( config["current_dac_enable"].IsDefined() )
    m_current_dac_enable = config["current_dac_enable"].as<unsigned int>();
  else{
    auto errorMsg = "\"current_dac_enable\" was not found in configuration of a LPGBT_ADC object";
    spdlog::apply_all([&](LoggerPtr l) { l->critical("{}: {}",m_name,errorMsg); });
    throw swamp::LPGBTADCYAMLMissingParameterException(errorMsg);
  }
  if( config["current_dac_value"].IsDefined() )
    m_current_dac_value = config["current_dac_value"].as<unsigned int>();
  else{
    auto errorMsg = "\"current_dac_value\" was not found in configuration of a LPGBT_ADC object";
    spdlog::apply_all([&](LoggerPtr l) { l->critical("{}: {}",m_name,errorMsg); });
    throw swamp::LPGBTADCYAMLMissingParameterException(errorMsg);
  }
  if( config["voltage_dac_enable"].IsDefined() )
    m_voltage_dac_enable = config["voltage_dac_enable"].as<unsigned int>();
  else{
    auto errorMsg = "\"voltage_dac_enable\" was not found in configuration of a LPGBT_ADC object";
    spdlog::apply_all([&](LoggerPtr l) { l->critical("{}: {}",m_name,errorMsg); });
    throw swamp::LPGBTADCYAMLMissingParameterException(errorMsg);
  }
  if( config["voltage_dac_value"].IsDefined() )
    m_voltage_dac_value = config["voltage_dac_value"].as<unsigned int>();
  else{
    auto errorMsg = "\"voltage_dac_value\" was not found in configuration of a LPGBT_ADC object";
    spdlog::apply_all([&](LoggerPtr l) { l->critical("{}: {}",m_name,errorMsg); });
    throw swamp::LPGBTADCYAMLMissingParameterException(errorMsg);
  }
  if( config["vref_tune"].IsDefined() )
    m_vref_tune = config["vref_tune"].as<unsigned int>();
  else{
    auto errorMsg = "\"vref_tune\" was not found in configuration of a LPGBT_ADC object";
    spdlog::apply_all([&](LoggerPtr l) { l->critical("{}: {}",m_name,errorMsg); });
    throw swamp::LPGBTADCYAMLMissingParameterException(errorMsg);
  }
  if( config["external_measurements"].IsDefined() )
    m_external_measurements = config["external_measurements"].as<std::vector<LPGBT_ADC_MEASUREMENT_CONFIG>>();
  else{ 
    auto errorMsg = "\"external_measurements\" was not found in configuration of a LPGBT_ADC object";
    spdlog::apply_all([&](LoggerPtr l) { l->critical("{}: {}",m_name,errorMsg); });
    throw swamp::LPGBTADCYAMLMissingParameterException(errorMsg);
  }
}

void LPGBT_ADC::ConfigureImpl(const YAML::Node& node)
{
  /*
    From lpgbt_v1.py vref_enable(self, enable=True, tune=0)
    """Controls the bandgap reference voltage generator
    
    Arguments:
    enable: reference generator state
    tune: reference voltage tuning word
    """
  */
  spdlog::apply_all([&](LoggerPtr l) { l->debug("{}: configuring (setting vrefenable, vreftune, dacconfig, currentdac ...)",m_name); });

  auto cntr = VREFCNTR::VREFENABLE_BIT_MASK;
  auto tune = m_vref_tune << VREFTUNE::VREFTUNE_OFFSET;
  m_carrier->write_reg(VREFCNTR::ADDRESS, cntr);
  m_carrier->write_reg(VREFTUNE::ADDRESS, tune);

  auto curdaccch = m_carrier->read_reg(CURDACCHN::ADDRESS);
  auto dacconfigh = m_carrier->read_reg(DACCONFIGH::ADDRESS);

  std::for_each(m_external_measurements.begin(),
		m_external_measurements.end(),
		[&](const auto& measurement){
		  auto pval = ADCPINMAP.find(measurement.channel_p)->second;
		  if( measurement.current_dac_ch_enable )
		    curdaccch |= 1 << pval;
		  else
		    curdaccch &= ~(1 << pval);
		});
  if( m_current_dac_enable )
    dacconfigh |= DACCONFIGH::CURDACENABLE_BIT_MASK;
  else
    dacconfigh &= (~DACCONFIGH::CURDACENABLE_BIT_MASK );

  m_carrier->write_reg(CURDACCHN::ADDRESS, curdaccch);
  m_carrier->write_reg(DACCONFIGH::ADDRESS, dacconfigh);
  m_carrier->write_reg(CURDACVALUE::ADDRESS, m_current_dac_value);
}

void LPGBT_ADC::AcquireImpl(unsigned int nSamples)
{
  auto acquire_adc_measurement = [&](const auto & measurement){

    std::vector<unsigned int> results;
    results.reserve(nSamples);
    
    auto inp = ADCPINMAP.find(measurement.channel_p)->second;
    auto inn = ADCPINMAP.find(measurement.channel_n)->second;
    auto gain = measurement.gain;
    auto mux = (inp << ADCSELECT::ADCINPSELECT_OFFSET | inn << ADCSELECT::ADCINNSELECT_OFFSET);
    //maybe need to configure ADC config in 2 steps with 1st enable and gain, and then adc convert 
    auto config = ADCCONFIG::ADCENABLE_BIT_MASK | (gain << ADCCONFIG::ADCGAINSELECT_OFFSET) ; 

    m_carrier->write_reg(ADCCONFIG::ADDRESS, config);
    m_carrier->write_reg(ADCSELECT::ADDRESS, mux);

    for( unsigned int sample(0); sample<nSamples; sample++ ){
      m_carrier->write_reg(ADCCONFIG::ADDRESS, config | ADCCONFIG::ADCCONVERT_BIT_MASK);
      Timer timer(0.001);
      while(true){
	auto status = m_carrier->read_regs( ADCSTATUSH::ADDRESS,2 );
	if( status[0] & ADCSTATUSH::ADCDONE_BIT_MASK ){
	  auto value = ((status[0] & ADCSTATUSH::ADCVALUE_BIT_MASK) << 8) | status[1];
	  results.push_back( value );
	  break;
	}
	if(timer.hasTimedOut()){
	  auto errorMsg = "Timeout while waiting for LPGBT_ADC conversion";
	  spdlog::apply_all([&](LoggerPtr l) { l->critical("{}: {}",m_name,errorMsg); });
	  m_carrier->unlock();
	  throw swamp::LPGBTADCConvertTimeoutException(errorMsg);
	}
      }
      //remove conversion bit  
      m_carrier->write_reg(ADCCONFIG::ADDRESS, config);
    }
    return results;
  };

  std::for_each( m_external_measurements.begin(), m_external_measurements.end(),
		 [&](const auto & measurement){
		   spdlog::apply_all([&](LoggerPtr l) { l->debug("{} : acquiring LPGBT ADC measurements {}",m_name,measurement.name); });

		   auto values = acquire_adc_measurement(measurement);
		   auto output = std::make_shared<ADCMeasurementOutput>( measurement.name, values );
		   // std::cout << "filling ADC buffer with values ";
		   fillADCMeasurementBuffer(output);
		   // std::for_each(output->rawValues.begin(),output->rawValues.end(),
		   // 		 [](const auto& rv){std::cout << rv << " ";});
		   // std::cout << std::endl;
		 });  
}
