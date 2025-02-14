#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ADC_TEST
#include <boost/test/unit_test.hpp>

#include "chipfactory.hpp"
#include "transportfactory.hpp"
#include "adcfactory.hpp"

BOOST_AUTO_TEST_SUITE (ADC_TEST)

BOOST_AUTO_TEST_CASE (LPGBT_ADC_TEST)
{
  auto cfg = YAML::LoadFile("test/config/test_train.yaml");
  auto logger = spdlog::get("console");
  spdlog::set_level(spdlog::level::info);

  TransportFactory trFactory;
  ChipFactory chFactory;
  ADCFactory adcFactory;
  YAML::Node transportcfg = cfg["transport"]["dummyTr"];
  YAML::Node chipcfg = cfg["chip"]["lpgbt_daq"];
 
  // lpgbt_adc transport needs a carrier
  auto achip = chFactory.Create("lpgbt","lpgbt_daq",chipcfg["cfg"]);  
  
  // lpgbt chip needs a transport, dummy transport does not need anything
  auto dummyT = trFactory.Create("dummy",
				 "dummyT",
				 transportcfg["cfg"]);

  spdlog::set_level(spdlog::level::debug);

  YAML::Node adccfg = cfg["adc"]["adc_daq"];

  auto adc = adcFactory.Create("lpgbt_adc",
			       "adc_daq",
			       adccfg["cfg"]);

  BOOST_TEST_CHECKPOINT( "SWAMP object instanciation done");
  
  achip->setTransport(dummyT);
  adc->setCarrier( achip );  
  BOOST_TEST_CHECKPOINT( "link between adcs and carriers done");

  YAML::Node node;
  adc->configure(node);
  adc->acquire(10);

  while(true){
    auto adcoutput = adc->getLatestADCMeasurement();
    if( adcoutput->name=="null" ) break;
    else{
      std::ostringstream os( std::ostringstream::ate );
      os.str("");
      std::for_each(adcoutput->rawValues.begin(),adcoutput->rawValues.end(),
		    [&os](const auto& rv){
		      os << rv << " ";
		    });
      spdlog::apply_all([&](LoggerPtr l) { l->info("Reading from ADC buffer: measurement of {} gave {}",adcoutput->name,os.str().c_str()); });
    }
  }

}

BOOST_AUTO_TEST_SUITE_END()
