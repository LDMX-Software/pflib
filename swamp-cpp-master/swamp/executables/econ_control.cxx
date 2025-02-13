#include "chipfactory.hpp"
#include "transportfactory.hpp"
#include "yaml_helper.hpp"

#include <boost/cstdint.hpp>
#include <boost/program_options.hpp>

#define READ 0
#define WRITE 1

int main(int argc, char** argv)
{
  int log_level, read_write;
  unsigned int value(0),i2cmFreq(3);
  bool compare_and_alert,configure_i2c_master;
  std::string hardware_cfg,lpgbt_name,econ_config_file,pname;
  std::vector<std::string> econs;
  try { 
    /** Define and parse the program options 
     */ 
    namespace po = boost::program_options; 
    po::options_description options("econ control options"); 
    options.add_options()
      ("help,h", "Print help messages")
      ("hardwareConfigFile,F", po::value<std::string>(&hardware_cfg)->default_value("test/config/sct_test.yaml"), "hardware configuration file")
      ("lpgbtName,l",          po::value<std::string>(&lpgbt_name)->default_value("lpgbt_trg_w"), "name of the lpgbt you want to control")
      ("econs,e",              po::value<std::vector<std::string> >()->multitoken(), "names of the econs you want to control")
      ("econdConfigFile,f",    po::value<std::string>(&econ_config_file), "path to configuration file use to read/write the econ")
      ("pname,p",              po::value<std::string>(&pname), "econ parammeter name to read or write, expected format : block.subblock.paramname (if a econConfigFile is given, pname will be ignored)")
      ("value,v",              po::value<unsigned int>(&value), "value to write in econ parameter (ignored if write flag==0 and if the econConfigFile is used)")
      ("read_write,w",         po::value<int>(&read_write)->default_value(READ), "read write flag, 1:write , 0:read")
      ("compareFlag,c",        po::value<bool>(&compare_and_alert)->default_value(false), "flag to set in read mode if you want to compare the read content with the initial configuration")
      ("configureI2CMaster,C", po::value<bool>(&configure_i2c_master)->default_value(false), "flag to set you want to reset and configure the I2C master before configuring/reading the ECON")
      ("I2CMFreq,IF",          po::value<unsigned int>(&i2cmFreq)->default_value(3), "Frequency of I2C master (only set when configureI2CMaster option is set)")
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
      econs = vm["econs"].as<std::vector<std::string> >();
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


  YAML::Node econ_config;
  if( econ_config_file.empty()==false )
    econ_config = YAML::LoadFile(econ_config_file);
  else{
    try{
      std::string delimiter(".");
      auto strvec = yaml_helper::strToYamlNodeNames(pname,delimiter);
      auto block_name = strvec[0];
      auto subblock_name = strvec[1];
      if(strvec.size()==2){
	econ_config[block_name][subblock_name] = value;
      }
      else if(strvec.size()==3){
	auto param_name = strvec[2];
	econ_config[block_name][subblock_name][param_name] = value;
      }
      else if(strvec.size()==4){
	auto param_name = strvec[2];
	auto param_id = strvec[3];
	econ_config[block_name][subblock_name][param_name][param_id] = value;
      }
    }
    catch( std::string& s ){
      spdlog::apply_all([&](LoggerPtr l) { l->critical("pname should have format : block_id.subblock_id.par_name -> Operation will NOT be performed"); });
      return 1; 
    }
  }

  for( auto econ_name : econs ){
    // econ and lpgbt_i2c instances:
    YAML::Node econ_cfg = cfg["chip"][ econ_name ];
    auto i2c_name = econ_cfg["transport"].as<std::string>();
    YAML::Node i2c_cfg = cfg["transport"][ i2c_name ];    
    auto econ = chFactory.Create(econ_cfg["type"].as<std::string>(),econ_name,econ_cfg["cfg"]);  
    auto i2c = trFactory.Create(i2c_cfg["type"].as<std::string>(),i2c_name,i2c_cfg["cfg"]);
    econ->setTransport(i2c);
    i2c->setCarrier(lpgbt);

    if( configure_i2c_master ){
      i2c->reset();
      YAML::Node node;
      node["clk_freq"] = i2cmFreq;
      node["scl_drive"] = 1;
      node["scl_pullup"] = 0;
      node["scl_drive_strength"] = 0;
      node["sda_pullup"] = 0;
      node["sda_drive_strength"] = 0;
      i2c->configure(node);
    }

    if( read_write==1 ){//write
      econ->configure( econ_config );
    }
    else{
      auto read_config = econ->read( econ_config );
      if( compare_and_alert ){
	auto expected = yaml_helper::compare(read_config,econ_config);
	if( expected )
	  spdlog::apply_all([&](LoggerPtr l) { l->info("{} : expected and read back configuration match",econ_name); });
	else
	  spdlog::apply_all([&](LoggerPtr l) { l->error("{} : expected and read back configuration DON'T match",econ_name); });
      }
      else std::cout << read_config << std::endl;
    }
  }  
  return 0;
}
