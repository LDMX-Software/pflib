#pragma once
#include <yaml-cpp/yaml.h>
#include "swamp_type.hpp"
#include "econ_parameter.hpp"

namespace ECON
{
  struct econ_helper
  {
    econ_helper(){m_chiptype=swamp::ChipType::ECON_D;}
    econ_helper( const swamp::ChipType chiptype ){ m_chiptype = chiptype; }
    
    unsigned int get_lsb_id(unsigned int val);
    // std::vector<std::string> strToYamlNodeNames(std::string str);
    ECONParam getParam(const std::string& pname);
    uint32_t getMaxValue(const std::string& pname);
    bool checkIfParamInRegMap(const std::string& pname);

    bool validate_configuration(const YAML::Node& node);
    std::map<uint16_t,uint8_t> defaultRegMap();
    std::set<uint16_t> i2cAddresses() const;
    YAML::Node regAndVal2Yaml( std::map<uint16_t,uint8_t> reg_and_val_map );

    swamp::ChipType m_chiptype;
  };  
}
