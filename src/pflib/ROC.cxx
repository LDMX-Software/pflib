#include "pflib/ROC.h"

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <iostream>

#include "pflib/packing/Hex.h"
#include "pflib/utility/load_integer_csv.h"
#include "register_maps/direct_access.h"

namespace pflib {

ROC::ROC(I2C& i2c, uint8_t roc_base_addr, const std::string& type_version)
    : i2c_{i2c},
      roc_base_{roc_base_addr},
      compiler_{Compiler::get(type_version)} {
  pflib_log(debug) << "base addr " << packing::hex(roc_base_);
}

std::vector<uint8_t> ROC::readPage(int ipage, int len) {
  i2c_.set_bus_speed(1400);

  // printf("i2c base address %#x\n", roc_base_);

  std::vector<uint8_t> retval;
  for (int i = 0; i < len; i++) {
    // set the address
    uint16_t fulladdr = (ipage << 5) | i;
    i2c_.write_byte(roc_base_ + 0, fulladdr & 0xFF);
    i2c_.write_byte(roc_base_ + 1, (fulladdr >> 8) & 0xFF);
    // now read
    retval.push_back(i2c_.read_byte(roc_base_ + 2));
  }
  return retval;
}

uint8_t ROC::getValue(int ipage, int offset) {
  i2c_.set_bus_speed(1400);

  // set the address
  uint16_t fulladdr = (ipage << 5) | offset;
  pflib_log(debug) << "ROC::getValue(" << ipage << ", " << offset
                   << ") -> full addr " << fulladdr;
  i2c_.write_byte(roc_base_ + 0, fulladdr & 0xFF);
  i2c_.write_byte(roc_base_ + 1, (fulladdr >> 8) & 0xFF);
  // now read
  return i2c_.read_byte(roc_base_ + 2);
}

std::vector<std::string> ROC::getDirectAccessParameters() {
  std::vector<std::string> names;
  names.reserve(pflib::DIRECT_ACCESS_PARAMETER_LUT.size());
  for (const auto& [name, spec] : pflib::DIRECT_ACCESS_PARAMETER_LUT) {
    names.push_back(name);
  }
  return names;
}

bool ROC::getDirectAccess(const std::string& name) {
  auto spec_it = pflib::DIRECT_ACCESS_PARAMETER_LUT.find(name);
  if (spec_it == pflib::DIRECT_ACCESS_PARAMETER_LUT.end()) {
    PFEXCEPTION_RAISE("BadName", "Direct Access parameter named '" + name +
                                     "' not found in mapping.");
  }
  return getDirectAccess(spec_it->second.reg, spec_it->second.bit_location);
}

bool ROC::getDirectAccess(int reg, int bit) {
  if (reg < 4 or reg > 7) {
    PFEXCEPTION_RAISE("BadReg",
                      "Direct access registers are the values 4, 5, 6, and 7.");
  }
  if (bit < 0 or bit > 7) {
    PFEXCEPTION_RAISE(
        "BadBit", "Direct access bit locations are the indices from 0 to 7.");
  }
  uint8_t reg_val = i2c_.read_byte(roc_base_ + reg);
  return ((reg_val >> bit) & 0b1) == 1;
}

void ROC::setDirectAccess(const std::string& name, bool val) {
  auto spec_it = pflib::DIRECT_ACCESS_PARAMETER_LUT.find(name);
  if (spec_it == pflib::DIRECT_ACCESS_PARAMETER_LUT.end()) {
    PFEXCEPTION_RAISE("BadName", "Direct Access parameter named '" + name +
                                     "' not found in mapping.");
  }
  setDirectAccess(spec_it->second.reg, spec_it->second.bit_location, val);
}

void ROC::setDirectAccess(int reg, int bit, bool val) {
  if (reg < 4 or reg > 7) {
    PFEXCEPTION_RAISE("BadReg",
                      "Direct access registers are the values 4, 5, 6, and 7.");
  }
  if (reg == 7) {
    PFEXCEPTION_RAISE("BadReg", "Direct access register 7 is read only.");
  }
  if (bit < 0 or bit > 7) {
    PFEXCEPTION_RAISE(
        "BadBit", "Direct access bit locations are the indices from 0 to 7.");
  }
  // get the register
  uint8_t reg_val = i2c_.read_byte(roc_base_ + reg);
  // clear the value in that bit position
  reg_val &= ~(static_cast<uint8_t>(1) << bit);
  // set the bit to the same as our value
  reg_val |= (static_cast<uint8_t>(val ? 1 : 0) << bit);
  // write the register back
  i2c_.write_byte(roc_base_ + reg, reg_val);
}

static const int TOP_PAGE = 45;
static const int MASK_RUN_MODE = 0x3;

void ROC::setRunMode(bool active) {
  uint8_t cval = getValue(TOP_PAGE, 0);
  uint8_t imask = 0xFF ^ MASK_RUN_MODE;
  cval &= imask;
  if (active) cval |= MASK_RUN_MODE;
  setValue(TOP_PAGE, 0, cval);
}

bool ROC::isRunMode() {
  uint8_t cval = getValue(TOP_PAGE, 0);
  return cval & MASK_RUN_MODE;
}

void ROC::setValue(int page, int offset, uint8_t value) {
  i2c_.set_bus_speed(1400);
  uint16_t fulladdr = (page << 5) | offset;
  i2c_.write_byte(roc_base_ + 0, fulladdr & 0xFF);
  i2c_.write_byte(roc_base_ + 1, (fulladdr >> 8) & 0xFF);
  i2c_.write_byte(roc_base_ + 2, value & 0xFF);
}

void ROC::setRegisters(const std::map<int, std::map<int, uint8_t>>& registers) {
  for (auto& page : registers) {
    for (auto& reg : page.second) {
      this->setValue(page.first, reg.first, reg.second);
    }
  }
}

std::map<int, std::map<int, uint8_t>> ROC::getRegisters(
    const std::map<int, std::map<int, uint8_t>>& selected) {
  std::map<int, std::map<int, uint8_t>> chip_reg;
  if (selected.empty()) {
    /**
     * When the input map is empty, then read all registers.
     */
    for (int page : compiler_.get_known_pages()) {
      std::vector<uint8_t> v = readPage(page, N_REGISTERS_PER_PAGE);
      for (int reg{0}; reg < N_REGISTERS_PER_PAGE; reg++) {
        chip_reg[page][reg] = v.at(reg);
      }
    }
  } else {
    /**
     * When the input map is not empty, only read the registers that are within
     * that mapping.
     */
    for (auto& page : selected) {
      int page_id = page.first;
      std::vector<uint8_t> on_chip_reg_values =
          readPage(page_id, N_REGISTERS_PER_PAGE);
      for (int i{0}; i < N_REGISTERS_PER_PAGE; i++) {
        // skip un-touched registers
        if (page.second.find(i) == page.second.end()) continue;
        chip_reg[page_id][i] = on_chip_reg_values.at(i);
      }
    }
  }
  return chip_reg;
}

void ROC::loadRegisters(const std::string& file_name) {
  utility::load_integer_csv(file_name, [this](const std::vector<int>& cells) {
    if (cells.size() == 3) {
      setValue(cells.at(0), cells.at(1), cells.at(2));
    } else {
      pflib_log(warn) << "Ignoring ROC CSV register settings line without "
                         "exactly three columns";
    }
  });
}

std::map<std::string, uint64_t> ROC::getParameters(const std::string& page) {
  /**
   * get registers corresponding to a page
   */
  std::string PAGE{upper_cp(page)};
  auto registers = compiler_.getRegisters(PAGE);

  /**
   * Get the values of these registers from the chip
   */
  auto chip_reg = getRegisters(registers);

  /**
   * De-compile the registers back into the parameter mapping
   *
   * We don't be careful here since we are skipping pages
   */
  auto chip_params = compiler_.decompile(chip_reg, false);

  return chip_params[PAGE];
}

// added by Josh to get the actual value out in the script
std::map<std::string, std::map<std::string, uint64_t>> ROC::readParameters(
    const std::map<std::string, std::map<std::string, uint64_t>>& parameters,
    bool print_values) {
  auto touched_registers = compiler_.compile(parameters);
  auto chip_reg{getRegisters(touched_registers)};
  // decompile with little_endian and not being careful since we are not trying
  // to read all the chip values
  std::map<std::string, std::map<std::string, uint64_t>> parameter_values =
      compiler_.decompile(chip_reg, false, true);
  // print by default
  if (print_values) {
    for (const auto& [page_name, params] : parameter_values) {
      printf("%s:\n", page_name.c_str());
      for (const auto& [param_name, value] : params) {
        printf("  %s: 0x%lx (%lu)\n", param_name.c_str(), value, value);
      }
    }
  }
  return parameter_values;
}
// also addded by Josh to finish matching what is in the ECON register reading
// functions.
uint64_t ROC::dumpParameter(const std::string& page, const std::string& param) {
  std::map<std::string, std::map<std::string, uint64_t>> p;
  p[page][param] = 0;
  auto values = this->readParameters(p, false);  // get the results
  return values[page][param];  // return the actual register value
}

std::map<std::string, std::map<std::string, uint64_t>> ROC::defaults() {
  return compiler_.defaults();
}

std::map<int, std::map<int, uint8_t>> ROC::applyParameters(
    const std::map<std::string, std::map<std::string, uint64_t>>& parameters) {
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

void ROC::loadParameters(const std::string& file_path, bool prepend_defaults) {
  if (prepend_defaults) {
    /**
     * If we prepend defaults, then ALL of the parameters will be
     * touched and so we do need to bother reading the current
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

void ROC::applyParameter(const std::string& page, const std::string& param,
                         const uint64_t& val) {
  std::map<std::string, std::map<std::string, uint64_t>> p;
  p[page][param] = val;
  this->applyParameters(p);
}

void ROC::dumpSettings(const std::string& filename, bool should_decompile) {
  if (filename.empty()) {
    PFEXCEPTION_RAISE("Filename", "No filename provided to dump roc settings.");
  }
  std::ofstream f{filename};
  if (not f.is_open()) {
    PFEXCEPTION_RAISE(
        "File", "Unable to open file " + filename + " in dump roc settings.");
  }

  if (should_decompile) {
    // read all the pages and store them in memory
    std::map<int, std::map<int, uint8_t>> register_values{getRegisters({})};

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
    // read all the pages and write to CSV while reading
    std::map<int, std::map<int, uint8_t>> register_values;
    for (int page : compiler_.get_known_pages()) {
      // all pages have up to 16 registers
      std::vector<uint8_t> v = readPage(page, N_REGISTERS_PER_PAGE);
      for (int reg{0}; reg < N_REGISTERS_PER_PAGE; reg++) {
        f << page << "," << reg << "," << packing::hex(v.at(reg)) << '\n';
      }
    }
  }

  f.flush();
}

ROC::TestParameters::TestParameters(
    ROC& roc, std::map<std::string, std::map<std::string, uint64_t>> new_params)
    : roc_{roc} {
  previous_registers_ = roc_.applyParameters(new_params);
}

ROC::TestParameters::~TestParameters() {
  roc_.setRegisters(previous_registers_);
}

ROC::TestParameters::Builder::Builder(ROC& roc) : parameters_{}, roc_{roc} {}

ROC::TestParameters::Builder& ROC::TestParameters::Builder::add(
    const std::string& page, const std::string& param, const uint64_t& val) {
  parameters_[page][param] = val;
  return *this;
}

ROC::TestParameters::Builder& ROC::TestParameters::Builder::add_all_channels(
    const std::string& param, const uint64_t& val) {
  for (int ch{0}; ch < 72; ch++) {
    add("CH_" + std::to_string(ch), param, val);
  }
  return *this;
}

ROC::TestParameters ROC::TestParameters::Builder::apply() {
  return TestParameters(roc_, parameters_);
}

ROC::TestParameters::Builder ROC::testParameters() {
  return ROC::TestParameters::Builder(*this);
}

}  // namespace pflib
