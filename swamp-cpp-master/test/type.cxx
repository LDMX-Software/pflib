#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SWAMP_TYPE_TEST
#include <boost/test/unit_test.hpp>

#include "chipfactory.hpp"
#include "gpiofactory.hpp"
#include "transportfactory.hpp"

BOOST_AUTO_TEST_SUITE (SWAMP_TYPE_TEST)

BOOST_AUTO_TEST_CASE (ENUM_VALIDATOR)
{
  auto LPGBT       = swamp::ChipType::LPGBT       ;
  auto LPGBT_D     = swamp::ChipType::LPGBT_D     ;
  auto LPGBT_T     = swamp::ChipType::LPGBT_T     ;
  auto LPGBT_D2    = swamp::ChipType::LPGBT_D2    ;
  auto HGCROC      = swamp::ChipType::HGCROC      ;
  auto HGCROC_Si   = swamp::ChipType::HGCROC_Si   ;
  auto HGCROC_SiPM = swamp::ChipType::HGCROC_SiPM ;
  auto ECON        = swamp::ChipType::ECON        ;
  auto ECON_D      = swamp::ChipType::ECON_D      ;
  auto ECON_T      = swamp::ChipType::ECON_T      ;
  auto VTRXP       = swamp::ChipType::VTRXP       ;
  auto GBT_SCA     = swamp::ChipType::GBT_SCA     ;

  auto logger = spdlog::get("console");
  spdlog::apply_all([&](LoggerPtr l) { l->info( "LPGBT = {:04x}"      , static_cast<int>(LPGBT) );  });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "LPGBT_D = {:04x}"    , static_cast<int>(LPGBT_D) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "LPGBT_T = {:04x}"    , static_cast<int>(LPGBT_T) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "LPGBT_D2 = {:04x}"   , static_cast<int>(LPGBT_D2) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "HGCROC = {:04x}"     , static_cast<int>(HGCROC) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "HGCROCSi = {:04x}"   , static_cast<int>(HGCROC_Si) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "HGCROCSiPM = {:04x}" , static_cast<int>(HGCROC_SiPM) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "ECON = {:04x}"       , static_cast<int>(ECON) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "ECOND = {:04x}"      , static_cast<int>(ECON_D) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "ECONT = {:04x}"      , static_cast<int>(ECON_T) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "VTRXP = {:04x}"      , static_cast<int>(VTRXP) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "GBT_SCA = {:04x}"    , static_cast<int>(GBT_SCA) ); });
  
  BOOST_REQUIRE( (LPGBT & LPGBT_D)==LPGBT );
  BOOST_REQUIRE( (LPGBT & LPGBT_T)==LPGBT );
  BOOST_REQUIRE( (LPGBT & LPGBT_D2)==LPGBT );
  BOOST_REQUIRE( (LPGBT_D & LPGBT_D2)==LPGBT );

  BOOST_REQUIRE( (HGCROC & HGCROC_Si)   == HGCROC );
  BOOST_REQUIRE( (HGCROC & HGCROC_SiPM) == HGCROC );

  BOOST_REQUIRE( (ECON & ECON_D) == ECON );
  BOOST_REQUIRE( (ECON & ECON_T) == ECON );
  
  BOOST_REQUIRE( (LPGBT & HGCROC)   == swamp::ChipType::GENERIC );
  BOOST_REQUIRE( (LPGBT & HGCROC_Si)   == swamp::ChipType::GENERIC );
  BOOST_REQUIRE( (LPGBT & HGCROC_SiPM) == swamp::ChipType::GENERIC );
  BOOST_REQUIRE( (LPGBT & ECON)     == swamp::ChipType::GENERIC );
  BOOST_REQUIRE( (LPGBT & VTRXP)    == swamp::ChipType::GENERIC );
  BOOST_REQUIRE( (LPGBT & GBT_SCA)  == swamp::ChipType::GENERIC );
  BOOST_REQUIRE( (HGCROC & ECON)    == swamp::ChipType::GENERIC );
  BOOST_REQUIRE( (HGCROC & VTRXP)   == swamp::ChipType::GENERIC );
  BOOST_REQUIRE( (HGCROC & GBT_SCA) == swamp::ChipType::GENERIC );
  BOOST_REQUIRE( (ECON & VTRXP)     == swamp::ChipType::GENERIC );
  BOOST_REQUIRE( (ECON & GBT_SCA)   == swamp::ChipType::GENERIC );
  BOOST_REQUIRE( (VTRXP & GBT_SCA)  == swamp::ChipType::GENERIC );


  auto GENERIC      = swamp::GPIOType::GENERIC;
  auto RSTB         = swamp::GPIOType::RSTB;
  auto PWR_EN       = swamp::GPIOType::PWR_EN;
  auto PWR_GD       = swamp::GPIOType::PWR_GD;
  auto PWR_GD_DCDC  = swamp::GPIOType::PWR_GD_DCDC;
  auto PWR_GD_LDO   = swamp::GPIOType::PWR_GD_LDO;
  auto HGCROC_RE    = swamp::GPIOType::HGCROC_RE;
  auto HGCROC_RE_SB = swamp::GPIOType::HGCROC_RE_SB; 
  auto HGCROC_RE_HB = swamp::GPIOType::HGCROC_RE_HB; 
  auto ECON_RE      = swamp::GPIOType::ECON_RE; 
  auto ECON_RE_SB   = swamp::GPIOType::ECON_RE_SB; 
  auto ECON_RE_HB   = swamp::GPIOType::ECON_RE_HB; 
  auto READY        = swamp::GPIOType::READY; 

  spdlog::apply_all([&](LoggerPtr l) { l->info( "GPIO GENERIC = {:04x}"    , static_cast<int>(GENERIC) );  });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "GPIO RSTB = {:04x}"       , static_cast<int>(RSTB) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "GPIO PWR_EN = {:04x}"     , static_cast<int>(PWR_EN) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "GPIO PWR_GD = {:04x}"     , static_cast<int>(PWR_GD) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "GPIO PWR_GD_DCDC = {:04x}", static_cast<int>(PWR_GD_DCDC) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "GPIO PWR_GD_LDO = {:04x}" , static_cast<int>(PWR_GD_LDO) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "GPIO HGCROC_RE = {:04x}"    , static_cast<int>(HGCROC_RE) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "GPIO HGCROC_RE_SB = {:04x}" , static_cast<int>(HGCROC_RE_SB) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "GPIO HGCROC_RE_HB = {:04x}" , static_cast<int>(HGCROC_RE_HB) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "GPIO ECON_RE = {:04x}"      , static_cast<int>(ECON_RE) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "GPIO ECON_RE_SB = {:04x}"   , static_cast<int>(ECON_RE_SB) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "GPIO ECON_RE_HB = {:04x}"   , static_cast<int>(ECON_RE_HB) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "GPIO READY = {:04x}"        , static_cast<int>(READY) ); });


  BOOST_REQUIRE( (RSTB & PWR_EN)    == swamp::GPIOType::GENERIC );
  BOOST_REQUIRE( (RSTB & PWR_GD)    == swamp::GPIOType::GENERIC );
  BOOST_REQUIRE( (RSTB & PWR_GD_DCDC) == swamp::GPIOType::GENERIC );
  BOOST_REQUIRE( (RSTB & PWR_GD_LDO)  == swamp::GPIOType::GENERIC );

  BOOST_REQUIRE( (PWR_EN & PWR_GD) == swamp::GPIOType::GENERIC );
  BOOST_REQUIRE( (PWR_GD & PWR_GD_DCDC) == swamp::GPIOType::PWR_GD );
  BOOST_REQUIRE( (PWR_GD & PWR_GD_LDO)  == swamp::GPIOType::PWR_GD );

  BOOST_REQUIRE( (RSTB & HGCROC_RE)    == swamp::GPIOType::GENERIC );
  BOOST_REQUIRE( (RSTB & HGCROC_RE_SB) == swamp::GPIOType::GENERIC );
  BOOST_REQUIRE( (RSTB & HGCROC_RE_HB) == swamp::GPIOType::GENERIC );
  BOOST_REQUIRE( (RSTB & ECON_RE)      == swamp::GPIOType::GENERIC );
  BOOST_REQUIRE( (RSTB & ECON_RE_SB)   == swamp::GPIOType::GENERIC );
  BOOST_REQUIRE( (RSTB & ECON_RE_HB)   == swamp::GPIOType::GENERIC );

  BOOST_REQUIRE( (HGCROC_RE & HGCROC_RE_SB) == swamp::GPIOType::HGCROC_RE );
  BOOST_REQUIRE( (HGCROC_RE & HGCROC_RE_HB) == swamp::GPIOType::HGCROC_RE );
  BOOST_REQUIRE( (HGCROC_RE & ECON_RE)    == swamp::GPIOType::GENERIC );

  BOOST_REQUIRE( (ECON_RE & ECON_RE_SB) == swamp::GPIOType::ECON_RE );
  BOOST_REQUIRE( (ECON_RE & ECON_RE_HB) == swamp::GPIOType::ECON_RE );

  auto TGENERIC      = swamp::TransportType::GENERIC;
  auto IC            = swamp::TransportType::IC;
  auto EC            = swamp::TransportType::EC;
  auto LPGBT_I2C     = swamp::TransportType::LPGBT_I2C;
  auto LPGBT_I2C_DAQ = swamp::TransportType::LPGBT_I2C_DAQ;
  auto LPGBT_I2C_TRG = swamp::TransportType::LPGBT_I2C_TRG;
  auto GBT_SCA_I2C   = swamp::TransportType::GBT_SCA_I2C;

  spdlog::apply_all([&](LoggerPtr l) { l->info( "Transport GENERIC = {:04x}"       , static_cast<int>(TGENERIC) );  });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "Transport IC = {:04x}"            , static_cast<int>(IC) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "Transport EC = {:04x}"            , static_cast<int>(EC) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "Transport LPGBT_I2C = {:04x}"     , static_cast<int>(LPGBT_I2C) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "Transport LPGBT_I2C_DAQ = {:04x}" , static_cast<int>(LPGBT_I2C_DAQ) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "Transport LPGBT_I2C_TRG = {:04x}" , static_cast<int>(LPGBT_I2C_TRG) ); });
  spdlog::apply_all([&](LoggerPtr l) { l->info( "Transport GBT_SCA_I2C = {:04x}"   , static_cast<int>(GBT_SCA_I2C) ); });


  BOOST_REQUIRE( (IC & EC)            == swamp::TransportType::GENERIC );
  BOOST_REQUIRE( (IC & LPGBT_I2C)     == swamp::TransportType::GENERIC );
  BOOST_REQUIRE( (IC & LPGBT_I2C_DAQ) == swamp::TransportType::GENERIC );
  BOOST_REQUIRE( (IC & LPGBT_I2C_TRG) == swamp::TransportType::GENERIC );
  BOOST_REQUIRE( (IC & GBT_SCA_I2C)   == swamp::TransportType::GENERIC );

  BOOST_REQUIRE( (LPGBT_I2C & LPGBT_I2C_DAQ) == swamp::TransportType::LPGBT_I2C );
  BOOST_REQUIRE( (LPGBT_I2C & LPGBT_I2C_TRG) == swamp::TransportType::LPGBT_I2C );  
}


BOOST_AUTO_TEST_CASE (CHECK_CHIP_TYPE)
{
  auto logger = spdlog::get("console");
  auto cfg = YAML::LoadFile("test/config/test_train.yaml");
  YAML::Node chip_cfg = cfg["chip"]["lpgbt_daq"];
  std::string chip_name("lpgbt_daq");
  ChipFactory chFactory;

  auto lpgbt_d = chFactory.Create(chip_cfg["type"].as<std::string>(),chip_name,chip_cfg["cfg"]);
  BOOST_REQUIRE( swamp::IsChipOfType(lpgbt_d,swamp::ChipType::GENERIC)==true );
  BOOST_REQUIRE( swamp::IsChipOfType(lpgbt_d,swamp::ChipType::LPGBT)==true );
  BOOST_REQUIRE( swamp::IsChipOfType(lpgbt_d,swamp::ChipType::LPGBT_D)==true );
  BOOST_REQUIRE( swamp::IsChipOfType(lpgbt_d,swamp::ChipType::LPGBT_T)==false );
  BOOST_REQUIRE( swamp::IsChipOfType(lpgbt_d,swamp::ChipType::LPGBT_D2)==false );
  BOOST_REQUIRE( swamp::IsChipOfType(lpgbt_d,swamp::ChipType::HGCROC)==false );
  BOOST_REQUIRE( swamp::IsChipOfType(lpgbt_d,swamp::ChipType::ECON)==false );
  BOOST_REQUIRE( swamp::IsChipOfType(lpgbt_d,swamp::ChipType::VTRXP)==false );
  BOOST_REQUIRE( swamp::IsChipOfType(lpgbt_d,swamp::ChipType::GBT_SCA)==false );

  chip_cfg["cfg"]["address"] = 0x71;
  chip_cfg["cfg"]["chipType"] = "lpgbt_t";
  auto lpgbt_t = chFactory.Create(chip_cfg["type"].as<std::string>(),chip_name,chip_cfg["cfg"]);
  BOOST_REQUIRE( swamp::IsChipOfType(lpgbt_t,swamp::ChipType::LPGBT)==true );
  BOOST_REQUIRE( swamp::IsChipOfType(lpgbt_t,swamp::ChipType::LPGBT_D)==false );
  BOOST_REQUIRE( swamp::IsChipOfType(lpgbt_t,swamp::ChipType::LPGBT_T)==true );
  BOOST_REQUIRE( swamp::IsChipOfType(lpgbt_t,swamp::ChipType::LPGBT_D2)==false );

  chip_cfg["cfg"]["address"] = 0x76;
  chip_cfg["cfg"]["chipType"] = "lpgbt_d2";
  auto lpgbt_d2 = chFactory.Create(chip_cfg["type"].as<std::string>(),chip_name,chip_cfg["cfg"]);
  BOOST_REQUIRE( swamp::IsChipOfType(lpgbt_d2,swamp::ChipType::LPGBT)==true );
  BOOST_REQUIRE( swamp::IsChipOfType(lpgbt_d2,swamp::ChipType::LPGBT_D)==false );
  BOOST_REQUIRE( swamp::IsChipOfType(lpgbt_d2,swamp::ChipType::LPGBT_T)==false );
  BOOST_REQUIRE( swamp::IsChipOfType(lpgbt_d2,swamp::ChipType::LPGBT_D2)==true );

  chip_cfg = cfg["chip"]["vtrx"];
  chip_name = std::string("vtrx");
  auto vtrx = chFactory.Create(chip_cfg["type"].as<std::string>(),chip_name,chip_cfg["cfg"]);
  BOOST_REQUIRE( swamp::IsChipOfType(vtrx,swamp::ChipType::VTRXP)==true );
  BOOST_REQUIRE( swamp::IsChipOfType(vtrx,swamp::ChipType::LPGBT)==false );
  BOOST_REQUIRE( swamp::IsChipOfType(vtrx,swamp::ChipType::HGCROC)==false );
  BOOST_REQUIRE( swamp::IsChipOfType(vtrx,swamp::ChipType::ECON)==false );
  BOOST_REQUIRE( swamp::IsChipOfType(vtrx,swamp::ChipType::GBT_SCA)==false );
  
  chip_cfg = cfg["chip"]["roc0_w0"];
  chip_name = std::string("roc0_w0");
  auto roc = chFactory.Create(chip_cfg["type"].as<std::string>(),chip_name,chip_cfg["cfg"]);
  BOOST_REQUIRE( swamp::IsChipOfType(roc,swamp::ChipType::HGCROC)==true );
  BOOST_REQUIRE( swamp::IsChipOfType(roc,swamp::ChipType::HGCROC_Si)==true );
  BOOST_REQUIRE( swamp::IsChipOfType(roc,swamp::ChipType::HGCROC_SiPM)==false );

  chip_cfg = cfg["chip"]["econd_w0"];
  chip_name = std::string("econd_w0");
  auto econd = chFactory.Create(chip_cfg["type"].as<std::string>(),chip_name,chip_cfg["cfg"]);
  BOOST_REQUIRE( swamp::IsChipOfType(econd,swamp::ChipType::ECON)==true );
  BOOST_REQUIRE( swamp::IsChipOfType(econd,swamp::ChipType::ECON_D)==true );
  BOOST_REQUIRE( swamp::IsChipOfType(econd,swamp::ChipType::ECON_T)==false );

  chip_cfg = cfg["chip"]["econt_w0"];
  chip_name = std::string("econt_w0");
  auto econt = chFactory.Create(chip_cfg["type"].as<std::string>(),chip_name,chip_cfg["cfg"]);
  BOOST_REQUIRE( swamp::IsChipOfType(econt,swamp::ChipType::ECON)==true );
  BOOST_REQUIRE( swamp::IsChipOfType(econt,swamp::ChipType::ECON_D)==false );
  BOOST_REQUIRE( swamp::IsChipOfType(econt,swamp::ChipType::ECON_T)==true );
}


BOOST_AUTO_TEST_CASE (CHECK_GPIO_TYPE)
{
  auto logger = spdlog::get("console");
  auto cfg = YAML::LoadFile("test/config/test_train.yaml")["gpio"];
  GPIOFactory gpioFactory;

  auto gpioName = "hgcroc_re_hb_w0";
  auto gpio_cfg = cfg["hgcroc_re_hb_w0"];
  
  auto gpio = gpioFactory.Create(gpio_cfg["type"].as<std::string>(),gpioName,gpio_cfg["cfg"]);  
  
  BOOST_REQUIRE( swamp::IsGPIOOfType(gpio,swamp::GPIOType::GENERIC)==true );
  BOOST_REQUIRE( swamp::IsGPIOOfType(gpio,swamp::GPIOType::HGCROC_RE)==true );
  BOOST_REQUIRE( swamp::IsGPIOOfType(gpio,swamp::GPIOType::HGCROC_RE_HB)==true );
  BOOST_REQUIRE( swamp::IsGPIOOfType(gpio,swamp::GPIOType::HGCROC_RE_SB)==false );
  BOOST_REQUIRE( swamp::IsGPIOOfType(gpio,swamp::GPIOType::RSTB)==false );
  BOOST_REQUIRE( swamp::IsGPIOOfType(gpio,swamp::GPIOType::PWR_EN)==false );
  BOOST_REQUIRE( swamp::IsGPIOOfType(gpio,swamp::GPIOType::ECON_RE)==false );

  gpio_cfg["cfg"]["gpioType"] = "econ_re_hb";
  gpio = gpioFactory.Create(gpio_cfg["type"].as<std::string>(),gpioName,gpio_cfg["cfg"]);
  BOOST_REQUIRE( swamp::IsGPIOOfType(gpio,swamp::GPIOType::GENERIC)==true );
  BOOST_REQUIRE( swamp::IsGPIOOfType(gpio,swamp::GPIOType::HGCROC_RE)==false );
  BOOST_REQUIRE( swamp::IsGPIOOfType(gpio,swamp::GPIOType::RSTB)==false );
  BOOST_REQUIRE( swamp::IsGPIOOfType(gpio,swamp::GPIOType::PWR_EN)==false );
  BOOST_REQUIRE( swamp::IsGPIOOfType(gpio,swamp::GPIOType::ECON_RE)==true );
  BOOST_REQUIRE( swamp::IsGPIOOfType(gpio,swamp::GPIOType::ECON_RE_HB)==true );
  BOOST_REQUIRE( swamp::IsGPIOOfType(gpio,swamp::GPIOType::ECON_RE_SB)==false );  
}

BOOST_AUTO_TEST_CASE (CHECK_TRANSPORT_TYPE)
{
  auto logger = spdlog::get("console");
  auto cfg = YAML::LoadFile("test/config/test_train.yaml")["transport"];
  TransportFactory transportFactory;

  auto transportName = "ic";
  auto tcfg = cfg["ic"];
  auto transport = transportFactory.Create(tcfg["type"].as<std::string>(),transportName,tcfg["cfg"]);  
  
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::GENERIC)==true );
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::IC)==true );
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::EC)==false );
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::LPGBT_I2C)==false );
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::LPGBT_I2C_DAQ)==false );
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::LPGBT_I2C_TRG)==false );
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::GBT_SCA_I2C)==false );

  transportName = "i2c_trg_w0";
  tcfg = cfg["i2c_trg_w0"];
  transport = transportFactory.Create(tcfg["type"].as<std::string>(),transportName,tcfg["cfg"]);  
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::GENERIC)==true );
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::IC)==false );
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::EC)==false );
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::LPGBT_I2C)==true );
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::LPGBT_I2C_DAQ)==false );
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::LPGBT_I2C_TRG)==true );
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::GBT_SCA_I2C)==false );

  tcfg["cfg"]["transportType"] = "lpgbt_i2c_daq";
  transport = transportFactory.Create(tcfg["type"].as<std::string>(),transportName,tcfg["cfg"]);  
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::GENERIC)==true );
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::IC)==false );
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::EC)==false );
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::LPGBT_I2C)==true );
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::LPGBT_I2C_DAQ)==true );
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::LPGBT_I2C_TRG)==false );
  BOOST_REQUIRE( swamp::IsTransportOfType(transport,swamp::TransportType::GBT_SCA_I2C)==false );
}

BOOST_AUTO_TEST_SUITE_END()
