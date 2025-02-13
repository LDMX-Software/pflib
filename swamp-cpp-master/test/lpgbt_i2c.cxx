#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LPGBT_TEST
#include <boost/test/unit_test.hpp>

#include "transportfactory.hpp"
#include "chipfactory.hpp"

BOOST_AUTO_TEST_SUITE (LPGBT_TEST)

BOOST_AUTO_TEST_CASE (LPGBT_I2C_TEST)
{
  auto cfg = YAML::LoadFile("test/config/test_train.yaml");
  auto logger = spdlog::get("console");
  spdlog::set_level(spdlog::level::info);
    
  YAML::Node transportcfg = cfg["transport"]["i2c_trg_w0"];
  YAML::Node chipcfg = cfg["chip"]["lpgbt_daq"];
  TransportFactory trFactory;
  ChipFactory chFactory;
  
  auto i2cw0 = trFactory.Create( transportcfg["type"].as<std::string>(),
				 "i2c_trg_w0",
				 transportcfg["cfg"]);
  // lpgbt_i2c transport needs a carrier
  auto achip = chFactory.Create("lpgbt","lpgbt_daq",chipcfg["cfg"]);  
  
  // lpgbt chip needs a transport, dummy transport does not need anything
  auto dummyT = trFactory.Create("dummy",
				 "dummyT",
				 transportcfg["cfg"]);
  BOOST_TEST_CHECKPOINT( "SWAMP object instanciation done");

  achip->setTransport(dummyT);
  i2cw0->setCarrier( achip );  
  BOOST_TEST_CHECKPOINT( "link between transports and carriers done");

  i2cw0->reset();
  BOOST_TEST_CHECKPOINT( "lpgbt_i2c reset");
  YAML::Node node;
  node["clk_freq"] = 3;
  node["scl_drive"] = 0;
  node["scl_pullup"] = 0;
  node["scl_drive_strength"] = 0;
  node["sda_pullup"] = 0;
  node["sda_drive_strength"] = 0;
  i2cw0->configure(node);

  BOOST_TEST_CHECKPOINT( "lpgbt_i2c reset and configured");
  
  unsigned int address=0x08;
  unsigned int reg_addr_width=2;
  unsigned int reg_addr=34;
  std::vector<unsigned int> reg_vals(100);
  unsigned int i=0;
  std::transform(reg_vals.begin(),reg_vals.end(),reg_vals.begin(),[&i](auto& v){i++; return i&0xFF;});

  i2cw0->write_reg(address,reg_vals[2]);
  BOOST_TEST_CHECKPOINT( "lpgbt_i2c write_reg passed");

  auto dummyval __attribute__((unused)) = i2cw0->read_reg(address);  
  BOOST_TEST_CHECKPOINT( "lpgbt_i2c read_reg passed");

  i2cw0->write_regs(address,reg_addr_width,reg_addr,reg_vals);
  BOOST_TEST_CHECKPOINT( "lpgbt_i2c write_regs passed");

  auto dummyvec __attribute__((unused)) = i2cw0->read_regs(address,reg_addr_width,reg_addr,100);  
  BOOST_TEST_CHECKPOINT( "lpgbt_i2c read_regs passed");

  dummyvec = i2cw0->read_regs(address,100);
  BOOST_TEST_CHECKPOINT( "lpgbt_i2c read_regs passed");
}

BOOST_AUTO_TEST_SUITE_END()
