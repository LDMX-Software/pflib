#include "chipfactory.hpp"
#include "transportfactory.hpp"
#include "gpiofactory.hpp"
#include "yaml_helper.hpp"

#include <boost/cstdint.hpp>
#include <boost/program_options.hpp>

#define DOWN 0
#define UP 1
#define READ 0
#define WRITE 1

int main(int argc, char** argv)
{
  int log_level, up_down, read_write;
  std::string hardware_cfg,lpgbt_name;
  std::vector<std::string> gpios;
  try { 
    /** Define and parse the program options 
     */ 
    namespace po = boost::program_options; 
    po::options_description options("lpgbt GPIO control options"); 
    options.add_options()
      ("help,h", "Print help messages")
      ("hardwareConfigFile,F", po::value<std::string>(&hardware_cfg)->default_value("test/config/sct_test.yaml"), "hardware configuration file")
      ("lpgbtName,l",          po::value<std::string>(&lpgbt_name)->default_value("lpgbt_trg_w"), "name of the lpgbt you want to control")
      ("gpios,g",              po::value<std::vector<std::string> >()->multitoken(), "names of the GPIOs you want to control")
      ("up_down,u",            po::value<int>(&up_down)->default_value(UP), "up/down value of the GPIO pin, 1:up , 0:down")
      ("read_write,w",         po::value<int>(&read_write)->default_value(READ), "read/write option, 1:write , 0:read")
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
      gpios = vm["gpios"].as<std::vector<std::string> >();
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
  GPIOFactory gpioFactory;

  // lpgbt and IC/EC instances:
  YAML::Node lpgbt_cfg = cfg["chip"][ lpgbt_name ];
  auto icec_name = lpgbt_cfg["transport"].as<std::string>();
  YAML::Node icec_cfg = cfg["transport"][ icec_name ];
  auto lpgbt = chFactory.Create(lpgbt_cfg["type"].as<std::string>(),lpgbt_name,lpgbt_cfg["cfg"]);  
  auto icec  = trFactory.Create(icec_cfg["type"].as<std::string>(),icec_name,icec_cfg["cfg"]);
  lpgbt->setTransport(icec);


  std::unordered_map<GPIO_Direction, std::string> dirMap = {
    {GPIO_Direction::INPUT , "Input"},
    {GPIO_Direction::OUTPUT, "Output"}
  };  
  
  for( auto gpio_name : gpios ){
    // roc and lpgbt_i2c instances:
    YAML::Node gpio_cfg = cfg["gpio"][ gpio_name ];
    auto gpio = gpioFactory.Create(gpio_cfg["type"].as<std::string>(),gpio_name,gpio_cfg["cfg"]);  
    gpio->setCarrier(lpgbt);
    gpio->set_direction();
    if(read_write==WRITE){
      if(up_down==UP){
        gpio->up();
      }
      else
        gpio->down();
    }
    else{
      auto readDir = gpio->get_direction();
      if(gpio->is_up())
	spdlog::apply_all([&](LoggerPtr l) { l->info("{} : is a {} and is up",gpio_name,dirMap[readDir]); });
      else
	spdlog::apply_all([&](LoggerPtr l) { l->info("{} : is a {} and is down",gpio_name,dirMap[readDir]); });
    }
  }
  return 0;
}
