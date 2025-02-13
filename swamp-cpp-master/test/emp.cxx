#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE EMP_TEST
#include <boost/test/unit_test.hpp>

#include "chipfactory.hpp"
#include "transportfactory.hpp"
#include "yaml_helper.hpp"

BOOST_AUTO_TEST_SUITE (EMP_TEST)


BOOST_AUTO_TEST_CASE (EMP_INSTANCE_TEST)
{
  auto cfg = YAML::LoadFile("test/config/emp_test.yaml");
  YAML::Node chip_cfg = cfg["train"]["chip"]["lpgbt_daq"];
  YAML::Node tr_cfg = cfg["train"]["transport"]["ic"];
  std::string chip_name("lpgbt_daq");
  ChipFactory chFactory;
  TransportFactory trFactory;
  auto chip = chFactory.Create(chip_cfg["type"].as<std::string>(),chip_name,chip_cfg["cfg"]);  
  auto transport = trFactory.Create(tr_cfg["type"].as<std::string>(),"ic",tr_cfg["cfg"]);

  chip->setTransport(transport);
  auto lpgbt_config = YAML::LoadFile("test/config/setup_daq_lpgbt.yaml");
  chip->configure(lpgbt_config);

  auto read_config = chip->read(lpgbt_config,true);
  BOOST_REQUIRE(yaml_helper::compare(read_config,lpgbt_config)==true);  
}

BOOST_AUTO_TEST_SUITE_END()
