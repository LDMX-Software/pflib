#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ECON_TEST
#include <boost/test/unit_test.hpp>

#include "chipfactory.hpp"
#include "transportfactory.hpp"
#include "yaml_helper.hpp"
#include <econ_helper.hpp>
#include <fstream>

BOOST_AUTO_TEST_SUITE (ECON_TEST)


BOOST_AUTO_TEST_CASE (ECOND_TEST)
{
  auto logger = spdlog::get("console");
  spdlog::set_level(spdlog::level::info);

  auto cfg = YAML::LoadFile("test/config/test_train.yaml");
  YAML::Node  econcfg = cfg["chip"]["econd_w0"];
  YAML::Node trcfg = cfg["transport"]["i2c_trg_w0"];
  std::string econname("econd_w0");
  ChipFactory chFactory;
  TransportFactory trFactory;
  auto aecon = chFactory.Create(econcfg["type"].as<std::string>(),econname,econcfg["cfg"]);  
  auto i2c = trFactory.Create("dummy","i2c_trg_w0",trcfg["cfg"]);  
  aecon->setTransport( i2c );

  auto full_config = YAML::LoadFile("test/config/econd_default.yaml");
  auto read_full_config = aecon->read(false);
  BOOST_REQUIRE(yaml_helper::compare(read_full_config,full_config)==true);

  auto init_config = YAML::LoadFile("test/config/init_econd.yaml");
  aecon->configure(init_config);
  auto read_config = aecon->read(init_config,false);
  std::ostringstream os(std::ostringstream::ate);
  os << read_config << std::endl;
  spdlog::apply_all([&](LoggerPtr l) { l->debug("ECOND_TEST readout init config : \n{}",os.str().c_str()); });      
  BOOST_REQUIRE(yaml_helper::compare(read_config,init_config)==true);
}

BOOST_AUTO_TEST_CASE (ECONT_TEST)
{
  auto logger = spdlog::get("console");
  spdlog::set_level(spdlog::level::info);

  auto cfg = YAML::LoadFile("test/config/test_train.yaml");
  YAML::Node  econcfg = cfg["chip"]["econt_w0"];
  YAML::Node trcfg = cfg["transport"]["i2c_trg_w0"];
  std::string econname("econt_w0");
  ChipFactory chFactory;
  TransportFactory trFactory;
  auto aecon = chFactory.Create(econcfg["type"].as<std::string>(),econname,econcfg["cfg"]);  
  auto i2c = trFactory.Create("dummy","i2c_trg_w0",trcfg["cfg"]);  
  aecon->setTransport( i2c );

  auto full_config = YAML::LoadFile("test/config/econt_default.yaml");
  auto read_full_config = aecon->read(false);
  BOOST_REQUIRE(yaml_helper::compare(read_full_config,full_config)==true);

  auto init_config = YAML::LoadFile("test/config/init_econt.yaml");
  aecon->configure(init_config);
  auto read_config = aecon->read(init_config,false);
  std::ostringstream os(std::ostringstream::ate);
  os << read_config << std::endl;
  spdlog::apply_all([&](LoggerPtr l) { l->debug("ECOND_TEST readout init config : \n{}",os.str().c_str()); });      
  BOOST_REQUIRE(yaml_helper::compare(read_config,init_config)==true);
}

BOOST_AUTO_TEST_SUITE_END()

