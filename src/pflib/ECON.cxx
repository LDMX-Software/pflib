#include "pflib/ECON.h"

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <iostream>

#include "pflib/packing/Hex.h"
#include "pflib/utility/load_integer_csv.h"

namespace pflib {

ECON::ECON(I2C& i2c, uint8_t econ_base_addr, const std::string& type_version)
    : i2c_{i2c},
      econ_base_{econ_base_addr},
      compiler_{Compiler::get(type_version)}
{
  econ_reg_nbytes_lut_ = compiler_.build_register_byte_lut();

  pflib_log(debug) << "ECON base addr " << packing::hex(econ_base_);
}

uint32_t getParam(const std::vector<uint8_t>& data, size_t shift, uint32_t mask) {
  // combine bytes into a little-endian integer
  // data[0] is the least significant byte, etc
  uint64_t value = 0;
  for (size_t i = 0; i < data.size(); ++i) {
    value |= (static_cast<uint64_t>(data[i]) << (8 * i));
  }
  
  // now extract the field
  return (value >> shift) & mask;
}

std::vector<uint8_t> newParam(std::vector<uint8_t> prev_value, uint16_t reg_addr, int nbytes, uint32_t mask, int shift, uint32_t value) {
  // combine bytes into a single integer
  uint32_t reg_val = 0;
  for (int i = 0; i < nbytes; ++i) {
    reg_val |= static_cast<uint32_t>(prev_value[i]) << (8 * i);
  }

  reg_val &= ~(mask << shift);
  reg_val |= ((value & mask) << shift);

  // split back into bytes
  std::vector<uint8_t> new_data;
  for (int i = 0; i < nbytes; ++i) {
    new_data.push_back(static_cast<uint8_t>((reg_val >> (8 * i)) & 0xFF));
  }

  printf("To update register 0x%04x: bits [%d:%d] set to 0x%x\n",
	 reg_addr, shift + static_cast<int>(log2(mask)), shift, reg_val);
  
  return new_data;  
}


static const uint64_t ADDR_RUNBIT = 0x03C5;
static const uint32_t MASK_RUNBIT = 1;
static const int      SHIFT_RUNBIT = 23;
static const int      NBYTES_RUNBIT = 3;
  
static const uint64_t ADDR_PUSMSTATE = 0x3DF;
static const uint32_t MASK_PUSMSTATE = 15;
static const int      SHIFT_PUSMSTATE = 0;
static const int      NBYTES_PUSMSTATE = 4;
  
static const uint64_t ADDR_FCMDSTATUS = 0x03AB;
static const int      NBYTES_FCMDSTATUS = 1;

static const uint64_t ADDR_FCMD = 0x03A7;
static const int      NBYTES_FCMD = 1;
static const int      SHIFT_FCMDEDGE = 0;
static const uint32_t MASK_FCMDEDGE = 1;
static const int      SHIFT_FCMDINVERT = 3;
static const uint32_t MASK_FCMDINVERT = 1;

void ECON::setRunMode(bool active) {
  if (active) {
    // set run bit to 1
    std::vector<uint8_t> RUNBIT_read = getValues(ADDR_RUNBIT, NBYTES_RUNBIT);
    std::vector<uint8_t> RUNBIT_one = newParam(RUNBIT_read, ADDR_RUNBIT, NBYTES_RUNBIT, MASK_RUNBIT, SHIFT_RUNBIT, 1);
    setValues(ADDR_RUNBIT, RUNBIT_one);
    
    // set EdgeSel to 0
    std::vector<uint8_t> FCMDEDGE_read = getValues(ADDR_FCMD, NBYTES_FCMD);
    std::vector<uint8_t> FCMDEDGE_zero = newParam(FCMDEDGE_read, ADDR_FCMD, NBYTES_FCMD, MASK_FCMDEDGE, SHIFT_FCMDEDGE, 0);
    // invert FCMD data to 1
    std::vector<uint8_t> FCMD_invert = newParam(FCMDEDGE_zero, ADDR_FCMD, NBYTES_FCMD, MASK_FCMDINVERT, SHIFT_FCMDINVERT, 1);
    setValues(ADDR_FCMD, FCMD_invert);
  }
}
  
bool ECON::isRunMode() {
  // Read 3-byte register at 0x03C5 and extract run bit
  std::vector<uint8_t> PUSM_read = getValues(ADDR_RUNBIT, NBYTES_RUNBIT);
  uint32_t pusm_run_value = getParam(PUSM_read, SHIFT_RUNBIT, MASK_RUNBIT);
  std::cout << "PUSM run value: " << pusm_run_value << std::endl;

  // Read 4-byte register at 0x03DF and extract PUSM state
  std::vector<uint8_t> PUSM_state = getValues(ADDR_PUSMSTATE, NBYTES_PUSMSTATE);
  uint32_t pusm_state_value = getParam(PUSM_state, SHIFT_PUSMSTATE, MASK_PUSMSTATE);
  std::cout << "PUSM state value: " << pusm_state_value << std::endl;
  
  return pusm_run_value == 1 && pusm_state_value == 8;
}
  
std::vector<uint8_t> ECON::getValues(int reg_addr, int nbytes) {
  if(nbytes < 1) {
    pflib_log(error) << "Invalid nbytes = " << nbytes;
    return {};
  }
  
  pflib_log(info) << "ECON::getValues(" << packing::hex(reg_addr) << ", " << nbytes << ")";

  std::vector<uint8_t> waddr;
  waddr.push_back(static_cast<uint8_t>((reg_addr >> 8) & 0xFF));
  waddr.push_back(static_cast<uint8_t>(reg_addr & 0xFF));       
  std::vector<uint8_t> data = i2c_.general_write_read(econ_base_, waddr, nbytes);

  //for (size_t i = 0; i < data.size(); i++) {
  //  printf("%02zu : %02x\n", i, data[i]);
  //}
  
  return data;
}

void ECON::setValue(int reg_addr, uint64_t value, int nbytes) {
  if (nbytes <= 0 || nbytes > 8) {
    pflib_log(error) << "ECON::setValue called with invalid nbytes = " << nbytes;
    return;
  }

  // split the value into bytes (little endian)
  std::vector<uint8_t> data;
  for (int i = 0; i < nbytes; ++i) {
    data.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
  }

  setValues(reg_addr, data);
}
  
void ECON::setValues(int reg_addr, const std::vector<uint8_t>& values) {
  if (values.empty()) {
    pflib_log(error) << "ECON::setValues called with empty data vector";
    return;
  }

  pflib_log(info) << "ECON::setValues(" 
		  << packing::hex(reg_addr) 
                  << ", nbytes = " << values.size() 
		  << ")";

  // write buffer
  std::vector<uint8_t> wbuf;
  wbuf.push_back(static_cast<uint8_t>((reg_addr >> 8) & 0xFF));
  wbuf.push_back(static_cast<uint8_t>(reg_addr & 0xFF));       
  wbuf.insert(wbuf.end(), values.begin(), values.end());

  // perform write
  i2c_.general_write_read(econ_base_, wbuf, values.size());

  // printf("Wrote %zu bytes to register 0x%04x:\n", values.size(), reg_addr);
  // for (size_t i = 0; i < values.size(); ++i) {
  //   printf("  %02zu : %02x\n", i, values[i]);
  // }
}

void ECON::setRegisters(const std::map<int, std::map<int, uint8_t>>& registers) {
  // registers[0] holds the actual register map
  const auto& reg_map = registers.at(0);

  // print every register addr and value pair
  for (const auto& [reg_addr, value] : reg_map) {
    printf("setValue(0x%04x, 0x%02x)\n", reg_addr, value);
  }

  // combine adjacent registers into one write
  std::vector<uint8_t> adjacent_vals;
  int start_addr = -1;
  int last_addr = -1;

  for (const auto& [addr, value] : reg_map) {
    if (start_addr == -1) {
      // first register
      start_addr = addr;
      last_addr = addr;
      adjacent_vals.push_back(value);
    } else if (addr == last_addr + 1) {
      // contiguous, keep adding
      adjacent_vals.push_back(value);
      last_addr = addr;
    } else {
      // non-contiguous → write previous block
      printf("start_addr 0x%04x, values:", start_addr);
      for (auto v : adjacent_vals) printf(" 0x%02X", v);
      printf("\n");

      this->setValues(start_addr, adjacent_vals);

      // reset for new block
      adjacent_vals.clear();
      adjacent_vals.push_back(value);
      start_addr = addr;
      last_addr = addr;
    }
  }

  // write the final block
  if (!adjacent_vals.empty()) {
    printf("start_addr 0x%04x, values:", start_addr);
    for (auto v : adjacent_vals) printf(" 0x%02X", v);
    printf("\n");
    this->setValues(start_addr, adjacent_vals);
  }
}
  
std::map<int, std::map<int, uint8_t>> ECON::getRegisters(
    const std::map<int, std::map<int, uint8_t>>& selected) {
  // output map: page_id -> (register address -> value)
  std::map<int, std::map<int, uint8_t>> chip_reg;
  const int page_id = 0;  // always page 0

  // read all registers from the chip into a single map
  std::map<int, uint8_t> all_regs;
  for (const auto& [reg_addr, nbytes] : econ_reg_nbytes_lut_) {
    std::vector<uint8_t> values = getValues(reg_addr, nbytes);
    for (int i = 0; i < values.size(); ++i) {
      printf("Read [0x%04x] = 0x%02x\n", reg_addr, values[i]);
      all_regs[reg_addr + i] = values[i];
    }
  }
  
  if (selected.empty()) {
    // if no specific registers are requested, copy all read registers
    for (const auto& [reg, val] : all_regs) {
      chip_reg[page_id][reg] = val;
    }
  } else {
    // only read the registers in selected[0]
    const auto& reg_map = selected.at(page_id);
    for (const auto& [reg, _] : reg_map) {
      auto it = all_regs.find(reg);
      if (it != all_regs.end()) {
        chip_reg[page_id][reg] = it->second;
      }
    }
  }
  
  return chip_reg;
}
    
std::map<int, std::map<int, uint8_t>> ECON::applyParameters(const std::map<std::string, std::map<std::string, uint64_t>>& parameters) {
  /**
   * 1. get registers YAML file contains by compiling without defaults
   */
  auto touched_registers = compiler_.compile(parameters);
  /**
   * 2. get the current register values on the chip which is
   */
  auto chip_reg{getRegisters(touched_registers)};
  // copy of current chip values to return
  auto ret_val = chip_reg;
  /**
   * 3. compile this parameter onto those register values
   *    we can use the lower-level compile here because the
   *    compile in step 1 checks that all of the page and param
   *    names are correct
   */
  for (auto& page : parameters) {
    std::string page_name = upper_cp(page.first);
    for (auto& param : page.second) {
      compiler_.compile(page_name, upper_cp(param.first), param.second,
                        chip_reg);
    }
  }
  /**
   * 4. put these updated values onto the chip
   */
  this->setRegisters(chip_reg);
  return ret_val;
}

void ECON::applyParameter(const std::string& param,
                          const uint64_t& val) {
  std::map<std::string, std::map<std::string, uint64_t>> p;
  p[0][param] = val;
  this->applyParameters(p);
}
  
void ECON::loadParameters(const std::string& file_path, bool prepend_defaults) {
  if (prepend_defaults) {
    /**
     * If we prepend defaults, then ALL of the parameters will be
     * touched and so we do NOT need to bother reading the current
     * values and overlaying the new ones, instead we jump straight
     * to setting the registers.
     */
    auto settings = compiler_.compile(file_path, true);
    setRegisters(settings);
  } else {
    /**
     * If we don't prepend the defaults, then we use the other applyParameters
     * function to overlay the parameters we passed on top of the ones currently
     * on the chip after extracting them from the YAML file.
     */
    std::map<std::string, std::map<std::string, uint64_t>> parameters;
    compiler_.extract(std::vector<std::string>{file_path}, parameters);
    applyParameters(parameters);
  }
}

void ECON::dumpSettings(const std::string& filename, bool should_decompile) {
  if (filename.empty()) {
    PFEXCEPTION_RAISE("Filename", "No filename provided to dump econ settings.");
  }
  std::ofstream f{filename};
  if (not f.is_open()) {
    PFEXCEPTION_RAISE(
        "File", "Unable to open file " + filename + " in dump econ settings.");
  }

  // read all the pages and store them in memory                                                                                                                                                                    
  std::map<int, std::map<int, uint8_t>> register_values{getRegisters({})};

  if (should_decompile) {
    /**
     * decompile while being careful since we knowingly are attempting
     * to read ALL of the parameters on the chip
     */
    std::map<std::string, std::map<std::string, uint64_t>> parameter_values =
        compiler_.decompile(register_values, true);

    YAML::Emitter out;
    out << YAML::BeginMap;
    for (const auto& page : parameter_values) {
      out << YAML::Key << page.first;
      out << YAML::Value << YAML::BeginMap;
      for (const auto& param : page.second) {
        out << YAML::Key << param.first << YAML::Value << param.second;
      }
      out << YAML::EndMap;
    }
    out << YAML::EndMap;

    f << out.c_str();
  } else {
    for (const auto& [_, regs] : register_values) {
      for (const auto& [reg, val] : regs) {
        f << reg << "," << packing::hex(val) << '\n';
      }
    }
  }

  f.flush();
}
  
ECON::TestParameters::TestParameters(
				    ECON& econ, std::map<std::string, std::map<std::string, uint64_t>> new_params)
  : econ_{econ} {
  previous_registers_ = econ_.applyParameters(new_params);
}

ECON::TestParameters::~TestParameters() {
  econ_.setRegisters(previous_registers_);
}

ECON::TestParameters::Builder::Builder(ECON& econ) : parameters_{}, econ_{econ} {}

ECON::TestParameters::Builder& ECON::TestParameters::Builder::add(
    const std::string& page, const std::string& param, const uint64_t& val) {
  parameters_[page][param] = val;
  return *this;
}

ECON::TestParameters ECON::TestParameters::Builder::apply() {
  return TestParameters(econ_, parameters_);
}

ECON::TestParameters::Builder ECON::testParameters() {
  return ECON::TestParameters::Builder(*this);
}

}  // namespace pflib
