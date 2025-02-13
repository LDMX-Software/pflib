#include "chipfactory.hpp"
#include "transportfactory.hpp"
#include "yaml_helper.hpp"

#include <boost/cstdint.hpp>
#include <boost/program_options.hpp>

#define READ 0
#define WRITE 1

int main(int argc, char** argv)
{
  int log_level;
  int read_write;
  bool compare_and_alert;
  std::string hardware_cfg,lpgbt_name,config_file;
  try { 
    /** Define and parse the program options 
     */ 
    namespace po = boost::program_options; 
    po::options_description options("lpgbt control options"); 
    options.add_options()
      ("help,h", "Print help messages")
      ("hardwareConfigFile,F", po::value<std::string>(&hardware_cfg)->default_value("test/config/sct_test.yaml"), "hardware configuration file")
      ("lpgbtName,l",          po::value<std::string>(&lpgbt_name)->default_value("daq_lpgbt"), "name of the lpgbt you want to control")
      ("read_write,r", po::value<int>(&read_write)->default_value(READ), "read write flag, 1:write , 0:read")
      ("logLevel,L", po::value<int>(&log_level)->default_value(2), "log level : 0-Trace; 1-Debug; 2-Info; 3-Warn; 4-Error; 5-Critical; 6-Off")
      ("configFile,f", po::value<std::string>(&config_file)->default_value("test/config/setup_daq_lpgbt.yaml"), "configuration file use to read/write the lpgbt")
      ("compareFlag,c", po::value<bool>(&compare_and_alert)->default_value(false), "flag to set in read mode if you want to compare the read content with the initial configuration")
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
  YAML::Node chip_cfg = cfg["chip"][ lpgbt_name ];

  auto transport_name = chip_cfg["transport"].as<std::string>();
  YAML::Node tr_cfg = cfg["transport"][ transport_name ];
 
  ChipFactory chFactory;
  TransportFactory trFactory;
  auto chip = chFactory.Create(chip_cfg["type"].as<std::string>(),lpgbt_name,chip_cfg["cfg"]);  
  auto transport = trFactory.Create(tr_cfg["type"].as<std::string>(),transport_name,tr_cfg["cfg"]);

  chip->setTransport(transport);

  auto lpgbt_config = YAML::LoadFile(config_file);


  if(read_write==WRITE){
    chip->configure(lpgbt_config);
  }
  else{
    auto read_config = chip->read(lpgbt_config,true);
    std::ostringstream os;
    os.str(""); os << read_config;
    spdlog::apply_all([&](LoggerPtr l) { l->info("read back configuration = \n{}",os.str().c_str()); });
    if( compare_and_alert ){
      auto expected = yaml_helper::compare(read_config,lpgbt_config);
      if( expected )
	spdlog::apply_all([&](LoggerPtr l) { l->info("expected and read back configuration match"); });
      else
	spdlog::apply_all([&](LoggerPtr l) { l->error("expected and read back configuration DON'T match"); });
    }
  }
  
  return 0;
}




