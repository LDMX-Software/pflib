#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LPGBT_TEST
#include <boost/test/unit_test.hpp>

#include "chipfactory.hpp"
#include "transportfactory.hpp"
#include "yaml_helper.hpp"

BOOST_AUTO_TEST_SUITE (LPGBT_TEST)

BOOST_AUTO_TEST_CASE (LPGBT_CONFIG_TEST)
{
  // spdlog::cfg::load_env_levels();
  auto logger = spdlog::get("console");

  auto cfg = YAML::LoadFile("test/config/test_train.yaml");
  YAML::Node chip_cfg = cfg["chip"]["lpgbt_daq"];
  YAML::Node tr_cfg = cfg["transport"]["ic"];
  std::string chip_name("lpgbt_daq");
  ChipFactory chFactory;
  TransportFactory trFactory;
  auto chip = chFactory.Create(chip_cfg["type"].as<std::string>(),chip_name,chip_cfg["cfg"]);  
  auto transport = trFactory.Create("dummy","ic",tr_cfg["cfg"]);

  chip->setTransport(transport);
  auto lpgbt_config = YAML::LoadFile("test/config/setup_daq_lpgbt.yaml");
  chip->configure(lpgbt_config);

  auto read_config = chip->read(lpgbt_config,false);
  auto nregs = read_config.size();
  BOOST_REQUIRE(yaml_helper::compare(read_config,lpgbt_config)==true);

  auto new_config = lpgbt_config;
  new_config.remove( "CLKGWAITTIME" );
  read_config = chip->read(new_config,false);
  BOOST_REQUIRE(yaml_helper::compare(read_config,new_config)==true);
  BOOST_REQUIRE(nregs-1==read_config.size());
}


BOOST_AUTO_TEST_SUITE_END()

