#include "pflib/lpgbt/lpGBT_Registers.h"
#include <map>

namespace pflib {
namespace lpgbt {
struct Register {
  uint16_t address;
  uint8_t mask;
  uint8_t offset;
};

static std::map<std::string,Register> g_registers;

static void def_reg(const std::string& name, uint16_t addr, uint8_t mask=0xFF, uint8_t offset=0) {
  Register r;
  r.address=addr;
  r.mask=mask;
  r.offset=offset;
  g_registers[name]=r;
}

#include "lpGBT_Registers_def.h"

bool findRegister(const std::string& registerName, uint16_t& addr, uint8_t& mask, uint8_t& offset) {
  if (g_registers.empty()) {
    populate_registers();
  }
  auto ptr=g_registers.find(registerName);
  if (ptr==g_registers.end()) return false;
  addr=ptr->second.address;
  mask=ptr->second.mask;
  offset=ptr->second.offset;
  return true;
}

}
}
