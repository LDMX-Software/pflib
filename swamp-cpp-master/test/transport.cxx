#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TRANSPORT_TEST
#include <boost/test/unit_test.hpp>

#include "transportfactory.hpp"
#include "chipfactory.hpp"

BOOST_AUTO_TEST_SUITE (TRANSPORT_TEST)

BOOST_AUTO_TEST_CASE (TRANSPORT_CREATION_TEST)
{
  // spdlog::cfg::load_env_levels();
  auto logger = spdlog::get("console");

  auto cfg = YAML::LoadFile("test/config/test_train.yaml");

  std::vector<std::string> transports{"dummyTr","i2c_trg_w0"};

  for( auto trname :transports){
    YAML::Node trcfg = cfg["transport"][trname];
    TransportFactory trFactory;
    auto tr = trFactory.Create(trcfg["type"].as<std::string>(),
			       trname,
			       trcfg["cfg"]);
    spdlog::apply_all([&](LoggerPtr l) { l->info("Creation of {} of type {} was successfull",trname,trcfg["type"].as<std::string>()); });
  }
  BOOST_TEST_CHECKPOINT( "Creation of all transports passed");
  std::cout << std::endl;
}

BOOST_AUTO_TEST_CASE (DUMMY_TEST)
{
  auto cfg = YAML::LoadFile("test/config/test_train.yaml");
  YAML::Node transportcfg = cfg["transport"]["dummyTr"];
  auto logger = spdlog::get("console");

  TransportFactory trFactory;
  auto dummyTransport = trFactory.Create(transportcfg["type"].as<std::string>(),
					 "dummyTr",
					 transportcfg["cfg"]);

  YAML::Node chipcfg = cfg["chip"]["dummyChip"];
  ChipFactory chFactory;
  auto achip = chFactory.Create(chipcfg["type"].as<std::string>(),
				"dummyChip",
				chipcfg["cfg"]);  
  dummyTransport->setCarrier( achip );

  unsigned int target_address=0x08;
  unsigned int reg_addr_width=2;
  unsigned int reg_addr=34;
  std::vector<unsigned int> reg_vals={0,1,2};
  dummyTransport->write_regs(target_address,reg_addr_width,reg_addr,reg_vals);
  auto dummyvec = dummyTransport->read_regs(target_address,reg_addr_width,reg_addr,10);  

  
  dummyTransport->write_reg(target_address,0xAC);
  dummyTransport->read_reg(target_address);


  std::vector<unsigned int> reg_addr_vec={0,10,20};
  std::vector<unsigned int> read_len_vec={5,3,18};
  auto ret_vec = dummyTransport->read_regs(target_address,reg_addr_width,reg_addr_vec,read_len_vec);
  std::for_each(ret_vec.begin(), ret_vec.end(), [](const auto&v){ std::cout << v << " "; } );
  std::cout << std::endl;

  spdlog::set_level(spdlog::level::trace);
  std::vector<std::vector<unsigned int>> reg_vals_vec={ {0,1,2,3,4}, {10,101,12}, {12,45,56,23,23}};
  dummyTransport->write_regs(target_address,reg_addr_width,reg_addr_vec,reg_vals_vec);
  
}
BOOST_AUTO_TEST_SUITE_END()
