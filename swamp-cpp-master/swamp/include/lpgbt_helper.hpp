#pragma once
#include <yaml-cpp/yaml.h>

#include "lpgbt_reg.hpp"
#include "lpgbt_reg_and_val.hpp"

namespace LPGBT
{
  struct lpgbt_helper{
    lpgbt_helper(){;}
    bool checkIfRegIsInRegMap(const std::string& regName);
    LPGBTReg getRegister(const std::string& regName);
    std::string getRegisterName(unsigned int regId);
    const YAML::Node reg_and_val_2_yaml(const std::vector<lpgbt_reg_and_val>& reg_and_val_vec);
    std::map<uint16_t,uint8_t> defaultRegMap();
    std::set<uint16_t> reg_addresses();
  };
}
