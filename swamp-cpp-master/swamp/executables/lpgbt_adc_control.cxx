#include "chipfactory.hpp"
#include "transportfactory.hpp"
#include "adcfactory.hpp"
#include "yaml_helper.hpp"

#include <boost/cstdint.hpp>
#include <boost/program_options.hpp>

int main(int argc, char** argv)
{
  int log_level;
  unsigned int nSamples;
  std::string hardware_cfg,lpgbt_name;
  std::vector<std::string> adcs;
  try { 
    /** Define and parse the program options 
     */ 
    namespace po = boost::program_options; 
    po::options_description options("lpgbt GPIO control options"); 
    options.add_options()
      ("help,h", "Print help messages")
      ("hardwareConfigFile,F", po::value<std::string>(&hardware_cfg)->default_value("test/config/sct_test.yaml"), "hardware configuration file")
      ("lpgbtName,l",          po::value<std::string>(&lpgbt_name)->default_value("lpgbt_trg_w"), "name of the lpgbt you want to control")
      ("adcs,a",               po::value<std::vector<std::string> >()->multitoken(), "names of the ADCs you want to control")
      ("nSamples,n",           po::value<unsigned int>(&nSamples)->default_value(1), "number of sample for each adc measurement")
      ("logLevel,L",           po::value<int>(&log_level)->default_value(2), "log level : 0-Trace; 1-Debug; 2-Info; 3-Warn; 4-Error; 5-Critical; 6-Off")
      ;
    po::options_description cmdline_options;
    cmdline_options.add(options);

    po::variables_map vm; 
    try { 
      po::store(po::parse_command_line(argc, argv, cmdline_options),  vm); 
      if ( vm.count("help")  ) { 
        std::cout << options   << std::endl; 
        return 0; 
      } 
      po::notify(vm);
      adcs = vm["adcs"].as<std::vector<std::string> >();
    }
    catch(po::error& e) { 
      std::cerr << "ERROR: " << e.what() << std::endl << std::endl; 
      std::cerr << options << std::endl; 
      return 1; 
    }
  }
  catch(std::exception& e) { 
    std::cerr << "Unhandled Exception reached the top of main: " 
              << e.what() << ", application will now exit" << std::endl; 
    return 2; 
  }   

  uhal::setLogLevelTo(uhal::Error());
  switch(log_level){
  case 6:
    spdlog::set_level(spdlog::level::off);
    break;
  case 5:
    spdlog::set_level(spdlog::level::critical);
    break;
  case 4:
    spdlog::set_level(spdlog::level::err);
    break;
  case 3:
    spdlog::set_level(spdlog::level::warn);
    break;
  case 2:
    spdlog::set_level(spdlog::level::info);
    break;
  case 1:
    spdlog::set_level(spdlog::level::debug);
    break;
  case 0:
    spdlog::set_level(spdlog::level::trace);
    break;
  }

  auto cfg = YAML::LoadFile(hardware_cfg);
  ChipFactory chFactory;
  TransportFactory trFactory;
  ADCFactory adcFactory;

  // lpgbt and IC/EC instances:
  YAML::Node lpgbt_cfg = cfg["chip"][ lpgbt_name ];
  auto icec_name = lpgbt_cfg["transport"].as<std::string>();
  YAML::Node icec_cfg = cfg["transport"][ icec_name ];
  auto lpgbt = chFactory.Create(lpgbt_cfg["type"].as<std::string>(),lpgbt_name,lpgbt_cfg["cfg"]);  
  auto icec  = trFactory.Create(icec_cfg["type"].as<std::string>(),icec_name,icec_cfg["cfg"]);
  lpgbt->setTransport(icec);
  
  for( auto adc_name : adcs ){
    // roc and lpgbt_i2c instances:
    YAML::Node adc_cfg = cfg["adc"][ adc_name ];
    auto adc = adcFactory.Create(adc_cfg["type"].as<std::string>(),adc_name,adc_cfg["cfg"]);  
    adc->setCarrier(lpgbt);

    YAML::Node node;
    adc->configure(node);
    adc->acquire(nSamples);
    
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
  return 0;
}
