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
  spdlog::set_level(spdlog::level::debug);

  auto cfg = YAML::LoadFile("test/config/test_train.yaml");
  YAML::Node chip_cfg = cfg["chip"]["vtrx"];
  YAML::Node tr_cfg = cfg["transport"]["i2c_trg_w0"];
  std::string chip_name("vtrx");
  ChipFactory chFactory;
  TransportFactory trFactory;
  auto chip = chFactory.Create(chip_cfg["type"].as<std::string>(),chip_name,chip_cfg["cfg"]);  
  auto transport = trFactory.Create("dummy","DUMMY",tr_cfg["cfg"]);

  chip->setTransport(transport);
  auto vtrx_config = YAML::LoadFile("test/config/init_vtrx.yaml");
  chip->configure(vtrx_config);
  BOOST_TEST_CHECKPOINT( "VTRXp configured");
  
  auto read_config = chip->read(vtrx_config,false);
  std::ostringstream os( std::ostringstream::ate );
  os.str(""); os << read_config ;
  spdlog::apply_all([&](LoggerPtr l) { l->debug("Readback (from cache) \n{}",os.str().c_str()); });
  auto expected = yaml_helper::compare(read_config,vtrx_config);
  BOOST_REQUIRE(expected==true);
  
  // auto nregs = read_config.size();

  // auto new_config = lpgbt_config;
  // new_config.remove( "CLKGWAITTIME" );
  // read_config = chip->read(new_config,false);
  // BOOST_REQUIRE(yaml_helper::compare(read_config,new_config)==true);
  // BOOST_REQUIRE(nregs-1==read_config.size());
}


BOOST_AUTO_TEST_SUITE_END()

