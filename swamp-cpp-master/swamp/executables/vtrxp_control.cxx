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
  unsigned int value(0);
  bool compare_and_alert;
  std::string hardware_cfg,lpgbt_name,vtrx_config_file,pname;
  std::vector<std::string> vtrxs;
  try { 
    /** Define and parse the program options 
     */ 
    namespace po = boost::program_options; 
    po::options_description options("vtrx control options"); 
    options.add_options()
      ("help,h", "Print help messages")
      ("hardwareConfigFile,F", po::value<std::string>(&hardware_cfg)->default_value("test/config/sct_test.yaml"), "hardware configuration file")
      ("lpgbtName,l",          po::value<std::string>(&lpgbt_name)->default_value("lpgbt_trg_w"), "name of the lpgbt you want to control")
      ("vtrxs,e",              po::value<std::vector<std::string> >()->multitoken(), "names of the VTRX+(s) you want to control")
      ("vtrxConfigFile,f",     po::value<std::string>(&vtrx_config_file), "path to configuration file use to read/write the VTRX+(s)")
      ("pname,p",              po::value<std::string>(&pname), "VTRX+ parammeter name to read or write, expected format : block.paramname or paramname (if a vtrxConfigFile is given, pname will be ignored)")
      ("value,v",              po::value<unsigned int>(&value), "value to write in VTRX+ parameter (ignored if write flag==0 and if the vtrxConfigFile is used)")
      ("read_write,w",         po::value<int>(&read_write)->default_value(READ), "read write flag, 1:write , 0:read")
      ("compareFlag,c",        po::value<bool>(&compare_and_alert)->default_value(false), "flag to set in read mode if you want to compare the read content with the initial configuration")
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
      vtrxs = vm["vtrxs"].as<std::vector<std::string> >();
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


  YAML::Node vtrx_config;
  if( vtrx_config_file.empty()==false )
    vtrx_config = YAML::LoadFile(vtrx_config_file);
  else{
    try{
      std::string delimiter(".");
      auto strvec = yaml_helper::strToYamlNodeNames(pname,delimiter);
      auto block_name = strvec[0];
      if(strvec.size()==2){
	auto param_name = strvec[1];
	vtrx_config[block_name][param_name] = value;
      }
      else
	vtrx_config[block_name] = value;
    }
    catch( std::string& s ){
      spdlog::apply_all([&](LoggerPtr l) { l->critical("pname should have format : block_id.par_name or par_name -> Operation will NOT be performed"); });
      return 1; 
    }
  }

  for( auto vtrx_name : vtrxs ){
    // vtrx and lpgbt_i2c instances:
    YAML::Node vtrx_cfg = cfg["chip"][ vtrx_name ];
    auto i2c_name = vtrx_cfg["transport"].as<std::string>();
    YAML::Node i2c_cfg = cfg["transport"][ i2c_name ];    
    auto vtrx = chFactory.Create(vtrx_cfg["type"].as<std::string>(),vtrx_name,vtrx_cfg["cfg"]);  
    auto i2c = trFactory.Create(i2c_cfg["type"].as<std::string>(),i2c_name,i2c_cfg["cfg"]);
    vtrx->setTransport(i2c);
    i2c->setCarrier(lpgbt);

    if( read_write==1 ){//write
      vtrx->configure( vtrx_config );
    }
    else{
      auto read_config = vtrx->read( vtrx_config );
      //std::cout << read_config << std::endl;
      if( compare_and_alert ){
	auto expected = yaml_helper::compare(read_config,vtrx_config);
	if( expected )
	  spdlog::apply_all([&](LoggerPtr l) { l->info("{} : expected and read back configuration match",vtrx_name); });
	else{
	  spdlog::apply_all([&](LoggerPtr l) { l->error("{} : expected and read back configuration DON'T match",vtrx_name); });
	  std::cout << read_config << std::endl;
	}
      }
    }
  }  
  return 0;
}
