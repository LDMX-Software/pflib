#pragma once
#include "roc_parameter.hpp"
#include "roc_reg_and_val.hpp"
#include <yaml-cpp/yaml.h>

namespace HGCROC
{
  class roc_helper{
  public:
    roc_helper(){;}
    uint32_t getMaxValue(const std::string& pname);
    uint32_t getDefaultValue(uint16_t r0, uint16_t r1);
    uint32_t getDefaultValue(uint16_t address);
    bool checkIfParamInRegMap(const std::string& pname);
    bool checkIfParamInDirectRegMap(const std::string& pname);
    bool validate_configuration( const YAML::Node& config );
    std::array<RocReg,4> getRegs(const std::string& pname);
    bool isValidReg(const RocReg& reg);
    std::map<uint16_t,uint8_t> defaultRegMap();
    std::set<uint16_t> i2cAddresses() const;
    YAML::Node regAndVal2Yaml( const std::vector<reg_and_val> &reg_and_val_vec );
    std::map<unsigned int,unsigned int> config2DirectRegisterTransaction(const YAML::Node& config);
    YAML::Node directRegAndVal2Yaml( const std::map<unsigned int,unsigned int> &addrAndValMap );
  };
}

