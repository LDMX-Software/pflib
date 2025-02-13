#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ROC_TEST
#include <boost/test/unit_test.hpp>

#include "chipfactory.hpp"
#include "transportfactory.hpp"
#include "yaml_helper.hpp"
#include <roc_helper.hpp>
// #include <fstream>

using RocPtr = std::shared_ptr<Roc>;

BOOST_AUTO_TEST_SUITE (ROC_TEST)


BOOST_AUTO_TEST_CASE (ROC_PARAM_TEST)
{
  auto helper = HGCROC::roc_helper();
  // spdlog::cfg::load_env_levels();
  auto cfg = YAML::LoadFile("test/config/test_train.yaml");
  std::string rocname("roc0_w0");
  YAML::Node roccfg = cfg["chip"][rocname];
  auto logger = spdlog::get("console");
  // spdlog::set_level(spdlog::level::debug);

  RocPtr roc = std::make_shared<Roc>(rocname,roccfg["cfg"]);

  std::vector<std::string> blocks{"DigitalHalf","DigitalHalf","Top","ch","GlobalAnalog","MasterTdc"};
  std::vector<int> subblocks{0,1,0,35,0,1};
  std::vector<std::string> pnames{"Bx_offset","CalibrationSC","BIAS_I_PLL_D","trim_inv","Cf_comp","BIAS_CAL_DAC_CTDC_P_D"};
  std::vector<int> expectedR0{57,100,163,99,232,68};
  std::vector<int> expectedR1{11,5,5,10,10,5};
  for(unsigned int testreg=0; testreg<blocks.size(); testreg++ ){
    auto block = blocks.at(testreg);
    auto subblock = subblocks.at(testreg);
    auto pname = pnames.at(testreg);
    auto fullname = block + "__" + std::to_string(subblock) + "__" + pname;

    auto found = helper.checkIfParamInRegMap(fullname);
    BOOST_REQUIRE( found==true );
    if( found ){
      auto regs = helper.getRegs(fullname);
      auto r0=regs.at(0).R0;
      auto r1=regs.at(0).R1;
      BOOST_REQUIRE(r0==expectedR0[testreg]);
      BOOST_REQUIRE(r1==expectedR1[testreg]);      
      spdlog::apply_all([&](LoggerPtr l) { l->debug("HGCRoc (block,subblock,pname,R0,R1) {}, {}, {}, {}, {}",block,subblock,pname,r0,r1); });      
    }
    else
      spdlog::apply_all([&](LoggerPtr l) { l->error("HGCRocParam (block,subblock,pname) {}, {}, {} not found in param_map",block,subblock,pname); });    
  }
}

BOOST_AUTO_TEST_CASE (ROC_CONFIG_FILE_TEST)
{
  auto logger = spdlog::get("console");

  auto cfg = YAML::LoadFile("test/config/test_train.yaml");
  YAML::Node  roccfg = cfg["chip"]["roc0_w0"];
  YAML::Node trcfg = cfg["transport"]["i2c_trg_w0"];
  std::string rocname("roc0_w0");
  ChipFactory chFactory;
  TransportFactory trFactory;
  auto aroc = chFactory.Create(roccfg["type"].as<std::string>(),rocname,roccfg["cfg"]);  
  auto i2c = trFactory.Create("dummy","i2c_trg_w0",trcfg["cfg"]);  
  aroc->setTransport( i2c );

  // spdlog::set_level(spdlog::level::debug);
  auto full_config = YAML::LoadFile("test/config/si-roc3b-poweron-default.yaml");
  auto read_full_config = aroc->read(full_config,false);
  BOOST_REQUIRE(yaml_helper::compare(read_full_config,full_config)==true);

  auto init_config = YAML::LoadFile("test/config/init_roc_3b.yaml");

  aroc->configure(init_config);
  auto read_config = aroc->read(init_config,false);
  std::cout << read_config << std::endl;
  BOOST_REQUIRE(yaml_helper::compare(read_config,init_config)==true);

  auto init_config_full_cache = YAML::LoadFile("test/config/init_roc3b_full_cache.yaml");
  auto read_config_full_cache = aroc->read(false);
  BOOST_REQUIRE(yaml_helper::compare(read_config_full_cache,init_config_full_cache)==true);
  // std::ofstream fout("toto.yaml");
  // fout << read_config_full_cache;
  // fout.close();
}

BOOST_AUTO_TEST_CASE (ROC_DIRECT_ACCESS_REGISTER_TEST)
{
  auto logger = spdlog::get("console");

  auto cfg = YAML::LoadFile("test/config/test_train.yaml");
  YAML::Node  roccfg = cfg["chip"]["roc0_w0"];
  YAML::Node trcfg = cfg["transport"]["i2c_trg_w0"];
  std::string rocname("roc0_w0");
  ChipFactory chFactory;
  TransportFactory trFactory;
  auto aroc = chFactory.Create(roccfg["type"].as<std::string>(),rocname,roccfg["cfg"]);  
  auto i2c = trFactory.Create("dummy","i2c_trg_w0",trcfg["cfg"]);  
  aroc->setTransport( i2c );

  spdlog::set_level(spdlog::level::debug);

  auto config = YAML::LoadFile("test/config/direct_roc_registers.yaml");
  aroc->configure(config);
  auto read_config = aroc->read(config);
  std::cout << read_config << std::endl;
  BOOST_REQUIRE(yaml_helper::compare(read_config,config)==true);
}

BOOST_AUTO_TEST_SUITE_END()

