#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE GPIO_TEST
#include <boost/test/unit_test.hpp>

#include "chipfactory.hpp"
#include "transportfactory.hpp"
#include "lpgbt_gpio.hpp"

BOOST_AUTO_TEST_SUITE (GPIO_TEST)

BOOST_AUTO_TEST_CASE (LPGBT_GPIO_TEST)
{
  auto cfg = YAML::LoadFile("test/config/test_train.yaml");
  auto logger = spdlog::get("console");
  spdlog::set_level(spdlog::level::debug);
    
  YAML::Node chipcfg = cfg["chip"]["lpgbt_daq"];
  TransportFactory trFactory;
  ChipFactory chFactory;

  auto achip = chFactory.Create("lpgbt","dummyChip",chipcfg["cfg"]);  

  // lpgbt chip needs a transport, dummy transport does not need anything
  auto dummyT = trFactory.Create("dummy",
				 "dummyT",
				 YAML::Node());

  YAML::Node gpiocfg = cfg["gpio"]["hgcroc_re_hb_w0"];
  LPGBT_GPIO gpio_pin("hgcroc_re_hb_w0",gpiocfg["cfg"]);
  
  BOOST_TEST_CHECKPOINT( "dummy carrier (dummyChip) and lpgbt_gpio (hgcroc_re_hb_w0) have been created");

  achip->setTransport(dummyT);

  gpio_pin.setCarrier( achip );
  gpio_pin.set_direction(GPIO_Direction::INPUT);
  gpio_pin.set_direction();

  gpio_pin.up();
  gpio_pin.down();
  gpio_pin.up();
  
  BOOST_TEST_CHECKPOINT( "lpgbt_gpio direction was set");
}

BOOST_AUTO_TEST_SUITE_END()
