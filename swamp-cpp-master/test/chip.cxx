#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE CHIP_TEST
#include <boost/test/unit_test.hpp>

#include "chipfactory.hpp"

BOOST_AUTO_TEST_SUITE (CHIP_TEST)

BOOST_AUTO_TEST_CASE (CHIP_CREATION_TEST)
{
  // spdlog::cfg::load_env_levels();
  auto logger = spdlog::get("console");

  auto cfg = YAML::LoadFile("test/config/test_train.yaml");

  std::vector<std::string> chips{"dummyChip","roc0_w0","lpgbt_daq"};

  for( auto chipname :chips){
    YAML::Node chipcfg = cfg["chip"][chipname];
    ChipFactory chFactory;
    auto achip = chFactory.Create(chipcfg["type"].as<std::string>(),
				  chipname,
				  chipcfg["cfg"]);
    spdlog::apply_all([&](LoggerPtr l) { l->info("Creation of {} of type {} was successfull",achip->name(),chipcfg["type"].as<std::string>()); });
  }
  BOOST_TEST_CHECKPOINT( "Creation of all chips passed");
  std::cout << std::endl;
}
BOOST_AUTO_TEST_SUITE_END()
