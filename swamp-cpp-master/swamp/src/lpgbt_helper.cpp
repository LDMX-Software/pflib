#include <algorithm>
#include "magic_enum.hpp"
#include "generated_lpgbt_register_map.hpp"
#include "spdlogger.hpp"

#include "lpgbt_helper.hpp"

namespace LPGBT
{
  bool lpgbt_helper::checkIfRegIsInRegMap(const std::string& regName)
  {
    auto r = magic_enum::enum_cast<REGISTER>(regName);
    if( !r.has_value() )
      return false;
    return true;
  }

  LPGBTReg lpgbt_helper::getRegister(const std::string& regName)
  {
    auto r = magic_enum::enum_cast<REGISTER>(regName);
    return LPGBTRegs.at( static_cast<int>(r.value()) );
  }

  std::string lpgbt_helper::getRegisterName(unsigned int regId)
  {
    auto lRegister = static_cast<REGISTER>(regId);
    auto name = magic_enum::enum_name(lRegister);
    return static_cast<std::string>(name);
  }

  const YAML::Node lpgbt_helper::reg_and_val_2_yaml(const std::vector<lpgbt_reg_and_val>& reg_and_val_vec)
  {
    YAML::Node node;
    std::for_each( reg_and_val_vec.cbegin(),
		   reg_and_val_vec.cend(),
		   [&](const auto& reg) {
		     auto address = reg.reg_address();
		     auto foundReg = std::find_if( LPGBTRegs.cbegin(),
						   LPGBTRegs.cend(),
						   [&](const auto &lpgbt_register){
						     return lpgbt_register.address == address;} );
		     if( foundReg==LPGBTRegs.end() )
		       spdlog::apply_all([&](LoggerPtr l) { l->critical("Problem in lpgbt_helper::reg_and_val_2_yaml, reg address {} was not found in the LPGBT reg map",address); });
		     else{
		       auto regId = std::distance(LPGBTRegs.cbegin(), foundReg);
		       auto name = getRegisterName(regId);
		       auto val  = reg.val();
		       node[ static_cast<std::string>(name) ] = val;
		     }
		   } );
    return node;
  }

  std::map<uint16_t,uint8_t> lpgbt_helper::defaultRegMap()
  {
    std::map<uint16_t,uint8_t> aMap;
    std::for_each( LPGBTRegs.cbegin(),
		   LPGBTRegs.cend(),
		   [&](const auto &lpgbt_register){
		     aMap[lpgbt_register.address]=0;
		   });
    return aMap;
  }

  std::set<uint16_t> lpgbt_helper::reg_addresses()
  {
    std::set<uint16_t> aSet;
    std::transform( LPGBTRegs.begin(),
		    LPGBTRegs.end(),
		    std::inserter(aSet,aSet.end()),
		    [&](const auto &lpgbt_register){
		      return lpgbt_register.address;
		    });
    return aSet;
  }
  
}
