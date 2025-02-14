#include "chipfactory.hpp"
#include "transportfactory.hpp"
#include "yaml_helper.hpp"

#include <boost/cstdint.hpp>
#include <boost/program_options.hpp>


int main(int argc, char** argv)
{
  std::string asicCfgFile;
  try { 
    /** Define and parse the program options 
     */ 
    namespace po = boost::program_options; 
    po::options_description options("options"); 
    options.add_options()
      ("help,h", "Print help messages")
      ("asicCfgFile,f", po::value<std::string>(&asicCfgFile)->default_value("test/config/asic-template.yaml"), "asic template file");
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
  spdlog::set_level(spdlog::level::debug);

  auto cfg = YAML::LoadFile(asicCfgFile);
  ChipFactory chFactory;

  // lpgbt and IC/EC instances:
  YAML::Node lpgbt_cfg = cfg["chip"][ "lpgbt_template" ];
  auto lpgbt = chFactory.Create("lpgbt", "lpgbt_template", lpgbt_cfg["cfg"]);

  YAML::Node roc_cfg = cfg["chip"][ "siroc_template" ];
  auto roc = chFactory.Create("roc", "siroc_template", roc_cfg["cfg"]);

  YAML::Node econt_cfg = cfg["chip"][ "econt_template" ];
  auto econt = chFactory.Create("econ","econt_template",econt_cfg["cfg"]);

  YAML::Node econd_cfg = cfg["chip"][ "econd_template" ];
  auto econd = chFactory.Create("econ","econd_template",econd_cfg["cfg"]);

  YAML::Node vtrxp_cfg = cfg["chip"][ "vtrxp_template" ];
  auto vtrxp = chFactory.Create("vtrxp","vtrxp_template",vtrxp_cfg["cfg"]);
  return 0;
}
