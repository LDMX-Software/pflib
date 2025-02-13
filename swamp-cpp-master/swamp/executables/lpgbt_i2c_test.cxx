#include "chipfactory.hpp"
#include "transportfactory.hpp"

#include <boost/cstdint.hpp>
#include <boost/program_options.hpp>

int main(int argc, char** argv)
{
  int log_level;
  unsigned int i2cmFreq(3),lpgbtSCLDrive(0),lpgbtSCLDriveStrength(0),lpgbtSDADriveStrength(0);
  std::string hardware_cfg,lpgbt_name;
  std::vector<std::string> i2cMasters;
  std::vector<uint16_t> i2cSlaveAddresses;
  try { 
    /** Define and parse the program options 
     */ 
    namespace po = boost::program_options; 
    po::options_description options("I2C test options"); 
    options.add_options()
      ("help,h", "Print help messages")
      ("hardwareConfigFile,f", po::value<std::string>(&hardware_cfg)->default_value("test/config/sct_test.yaml"), "hardware configuration file")
      ("lpgbtName,l",          po::value<std::string>(&lpgbt_name)->default_value("lpgbt_trg_w"), "name of the lpgbt you want to control")
      ("I2CMasters,M",         po::value<std::vector<std::string> >()->multitoken(), "I2C buses you want to test")
      ("I2CMFreq,F",           po::value<unsigned int>(&i2cmFreq)->default_value(3), "Frequency of I2C master (only set when configureI2CMaster option is set)")
      ("SCLDrive,D",           po::value<unsigned int>(&lpgbtSCLDrive)->default_value(0), "enable to set LPGBT to fully drive the SCL line")
      ("SCLDriveStrength,C",   po::value<unsigned int>(&lpgbtSCLDriveStrength)->default_value(0), "enable to set SCL LPGBT drive to 10 mA instead of 3 mA")
      ("SDADriveStrength,A",   po::value<unsigned int>(&lpgbtSDADriveStrength)->default_value(0), "enable to set SDA LPGBT drive to 10 mA instead of 3 mA")
      ("SlaveAddresses,S",     po::value<std::vector<uint16_t> >()->multitoken(), "I2C slave addresses you want to test")
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
      i2cMasters = vm["I2CMasters"].as<std::vector<std::string> >();
      i2cSlaveAddresses = vm["SlaveAddresses"].as<std::vector<uint16_t> >();
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

  // lpgbt and IC/EC instances:
  YAML::Node lpgbt_cfg = cfg["chip"][ lpgbt_name ];
  auto icec_name = lpgbt_cfg["transport"].as<std::string>();
  YAML::Node icec_cfg = cfg["transport"][ icec_name ];
  auto lpgbt = chFactory.Create(lpgbt_cfg["type"].as<std::string>(),lpgbt_name,lpgbt_cfg["cfg"]);  
  auto icec  = trFactory.Create(icec_cfg["type"].as<std::string>(),icec_name,icec_cfg["cfg"]);
  lpgbt->setTransport(icec);

  unsigned int reg_address_width=2;

  std::vector<unsigned int> data{0x0,0xf0,0x0f,0xff,0xaa,0xcc,0x33,0x55,0x8,0x9};
  unsigned int regAddress=0;
  
  for( auto i2cMaster : i2cMasters ){
    YAML::Node i2c_cfg = cfg["transport"][ i2cMaster ];    
    auto i2c = trFactory.Create(i2c_cfg["type"].as<std::string>(),i2cMaster,i2c_cfg["cfg"]);
    i2c->setCarrier(lpgbt);
    i2c->reset();
    YAML::Node node;
    node["clk_freq"] = i2cmFreq;
    node["scl_drive"] = lpgbtSCLDrive;
    node["scl_pullup"] = 0;
    node["scl_drive_strength"] = lpgbtSCLDriveStrength;
    node["sda_pullup"] = 0;
    node["sda_drive_strength"] = lpgbtSDADriveStrength;
    i2c->configure(node);

    for( auto address : i2cSlaveAddresses ){
      i2c->write_regs(address,reg_address_width,regAddress,data);
      i2c->read_regs(address,reg_address_width,regAddress,data.size());
    }
  }

  
  return 0;
}
