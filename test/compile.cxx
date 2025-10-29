#define BOOST_TEST_DYN_LINK
#include "pflib/Compile.h"

#include <boost/test/unit_test.hpp>
#include <fstream>
#include <iomanip>

#include "pflib/Exception.h"

BOOST_AUTO_TEST_SUITE(compile)

BOOST_AUTO_TEST_CASE(get_by_name) {
  BOOST_CHECK_THROW(pflib::Compiler::get("sipm_v3"), pflib::Exception);
  BOOST_CHECK_NO_THROW(pflib::Compiler::get("sipm_rocv3"));
}

BOOST_AUTO_TEST_CASE(single_register_rocv3) {
  pflib::Compiler c = pflib::Compiler::get("sipm_rocv3");
  std::map<int, std::map<int, uint8_t>> registers, expected;
  c.compile("CH_45", "SEL_TRIG_TOA", 1, registers);
  expected[11][1] = 0b00000010;
  BOOST_CHECK_MESSAGE(registers == expected, "single-bit parameter setting");
  c.compile("CH_45", "TRIM_TOA", 0b101010, registers);
  expected[11][1] = 0b10101010;
  BOOST_CHECK_MESSAGE(registers == expected, "multi-bit parameter setting");
  c.compile("CH_45", "TRIM_TOA", 0, registers);
  expected[11][1] = 0b00000010;
  BOOST_CHECK_MESSAGE(registers == expected,
                      "multi-bit parameter setting not overwriting other "
                      "parameters in register");
  BOOST_TEST_INFO("too-large values throw exceptions");
  BOOST_CHECK_THROW(c.compile("CH_45", "TRIM_TOA", 255, registers),
                    pflib::Exception);
  BOOST_TEST_INFO("negative values throw exceptions");
  BOOST_CHECK_THROW(c.compile("CH_45", "TRIM_TOA", -12, registers),
                    pflib::Exception);
}

BOOST_AUTO_TEST_CASE(one_param_multi_registers) {
  pflib::Compiler c = pflib::Compiler::get("sipm_rocv3");
  std::map<int, std::map<int, uint8_t>> registers, expected;
  expected[89][24] = 0b00110011;
  expected[89][26] = 0b10110000;
  c.compile("DIGITALHALF_0", "BX_TRIGGER", 0b101100110011, registers);
  BOOST_CHECK_MESSAGE(registers == expected,
                      "set parameter across multiple registers");
  c.compile("DIGITALHALF_0", "BX_OFFSET", 0b110111001100, registers);
  // register 24 value inherited from earlier
  expected[89][25] = 0b11001100;
  expected[89][26] = 0b10111101;
  BOOST_CHECK_MESSAGE(registers == expected,
                      "set parameter across multiple registers without "
                      "overwriting other parameters in register");

  BOOST_TEST_INFO("too-large values throw exceptions");
  BOOST_CHECK_THROW(
      c.compile("DIGITALHALF_0", "BX_TRIGGER", 1 << 13, registers),
      pflib::Exception);
}

BOOST_AUTO_TEST_CASE(big_32bit_params) {
  /**
   * There are a few parameters that are a full 32 bits
   * and I want to make sure we can handle them.
   *
   * For example, DIGITALHALF_1.SC_TESTRAM is 32 bits split
   * across four registers. (page 43, registers 20-23)
   */
  pflib::Compiler c = pflib::Compiler::get("sipm_rocv3");
  std::map<int, std::map<int, uint8_t>> registers, expected;
  expected[43][20] = 0xab;
  expected[43][21] = 0xcd;
  expected[43][22] = 0xee;
  expected[43][23] = 0xff;
  c.compile("DIGITALHALF_1", "SC_TESTRAM", 0xffeecdab, registers);
  BOOST_CHECK(registers == expected);
}

BOOST_AUTO_TEST_CASE(overwrite_with_later_settings) {
  pflib::Compiler c = pflib::Compiler::get("sipm_rocv3");
  std::string test_filepath{"settings.yaml"}, test_overwrite{"overwrite.yaml"};

  {
    std::ofstream test_yaml{"settings.yaml"};
    test_yaml << "ch_*:\n"
              << "    channel_off: 1\n"
              << std::endl;
  }

  {
    std::ofstream test_yaml{"overwrite.yaml"};
    test_yaml << "ch_42:\n"
              << "    channel_off: 0\n"
              << std::endl;
  }

  std::map<std::string, std::map<std::string, uint64_t>> settings, expected;

  for (int ch{0}; ch < 72; ch++) {
    expected["CH_" + std::to_string(ch)]["CHANNEL_OFF"] = (ch == 42) ? 0 : 1;
  }
  c.extract({test_filepath, test_overwrite}, settings);
  BOOST_CHECK_MESSAGE(settings == expected,
                      "overwrite parameter with subsequent settings file");
}

BOOST_AUTO_TEST_CASE(glob_pages) {
  pflib::Compiler c = pflib::Compiler::get("sipm_rocv3");
  std::string test_filepath{"settings.yaml"};

  {
    std::ofstream test_yaml{"settings.yaml"};
    test_yaml << "ch_*:\n"
              << "    channel_off: 1\n"
              << std::endl;
  }

  std::map<std::string, std::map<std::string, uint64_t>> settings, expected;

  for (int ch{0}; ch < 72; ch++) {
    expected["CH_" + std::to_string(ch)]["CHANNEL_OFF"] = 1;
  }

  c.extract({test_filepath}, settings);
  BOOST_CHECK_MESSAGE(settings == expected,
                      "broadcast single parameter to all channel pages");
}

BOOST_AUTO_TEST_CASE(single_register_econd) {
  pflib::Compiler c = pflib::Compiler::get("econd");
  // Note: this map is going to return a vector where the lowest address
  // (0x0389) gets the least significant byte (rightmost) i.e. 0 and the highest
  // address (0x0390) gets the most significant byte (leftmost) i.e. 0xff
  std::map<int, std::map<int, uint8_t>> registers, expected;
  expected[0][0x0389] = 0;
  expected[0][0x038a] = 0;
  expected[0][0x038b] = 0;
  expected[0][0x038c] = 0;
  expected[0][0x038d] = 0xff;
  expected[0][0x038e] = 0xff;
  expected[0][0x038f] = 0xff;
  expected[0][0x0390] = 0xff;

  uint64_t mask_val = 0xFFFFFFFF00000000ULL;

  // Call compile() with uint64_t value
  c.compile("ALIGNER", "GLOBAL_MATCH_MASK_VAL", mask_val, registers);

  // The registers are already in the form that we need it in ECON (little
  // endian first), so we can just flatten it out
  std::vector<uint8_t> data;
  int page = 0;
  for (const auto& [addr, val] : registers[page]) {
    data.push_back(val);
  }

  /*
  for (const auto& [page_id, reg_map] : registers) {
    printf("Page %d:", page_id);
    for (const auto& [addr, value] : reg_map)
      printf(" [0x%04X]=0x%02X", addr, value);
    printf("\n");
  }

  for (uint8_t byte : data)
    printf("0x%02X ", byte);
  printf("\n");
  */

  BOOST_CHECK(registers == expected);
}

BOOST_AUTO_TEST_CASE(extract_largehexstr_econd) {
  pflib::Compiler c = pflib::Compiler::get("econd");

  std::string yaml_path = "aligner_test.yaml";
  {
    std::ofstream yaml_file{yaml_path};
    yaml_file << "ALIGNER:\n"
              << "  GLOBAL_MATCH_MASK_VAL: 0xFFFFFFFF00000000\n";
  }

  std::map<std::string, std::map<std::string, uint64_t>> settings, expected;
  expected["ALIGNER"]["GLOBAL_MATCH_MASK_VAL"] = 0xFFFFFFFF00000000ULL;
  c.extract({yaml_path}, settings);

  BOOST_CHECK_MESSAGE(settings == expected,
                      "ALIGNER.GLOBAL_MATCH_MASK_VAL mismatch");
}

BOOST_AUTO_TEST_CASE(extract_largeint_econd) {
  pflib::Compiler c = pflib::Compiler::get("econd");

  std::string yaml_path = "aligner_test.yaml";
  {
    std::ofstream yaml_file{yaml_path};
    yaml_file << "ALIGNER:\n"
              << "  GLOBAL_MATCH_MASK_VAL: '18446744069414584320'\n";
  }

  std::map<std::string, std::map<std::string, uint64_t>> settings, expected;
  expected["ALIGNER"]["GLOBAL_MATCH_MASK_VAL"] = 0xFFFFFFFF00000000ULL;
  c.extract({yaml_path}, settings);

  BOOST_CHECK_MESSAGE(settings == expected,
                      "ALIGNER.GLOBAL_MATCH_MASK_VAL mismatch");
}

BOOST_AUTO_TEST_CASE(full_lut_econd_decompile) {
  pflib::Compiler c = pflib::Compiler::get("econd_test");
  // pflib::Compiler c = pflib::Compiler::get("econd");

  std::map<int, std::map<int, uint8_t>> settings;
  settings[0][0x389] = 0xff;
  settings[0][0x38a] = 0;
  settings[0][0x38b] = 0;
  settings[0][0x38c] = 0;
  settings[0][0x38d] = 0;
  settings[0][0x38e] = 0;
  settings[0][0x38f] = 0;
  settings[0][0x390] = 0;

  settings[0][0x3c5] = 0;
  settings[0][0x3c6] = 0;
  settings[0][0x3c7] = 0x80;

  settings[0][0xf2c] = 0;
  settings[0][0xf2d] = 0x20;
  settings[0][0xf2e] = 0x92;
  settings[0][0xf2f] = 0xaa;
  settings[0][0xf30] = 0xaa;
  settings[0][0xf31] = 0x2a;
  settings[0][0xf32] = 0xf3;

  settings[0][0x0452] = 0xab;
  settings[0][0x0453] = 0x89;
  settings[0][0x0454] = 0x67;
  settings[0][0x0455] = 0x45;
  settings[0][0x0456] = 0x23;
  settings[0][0x0457] = 0x01;
  
  auto chip_params = c.decompile(settings, true, true);

  for (const auto& [page_name, params] : chip_params) {
    std::cout << "Page: " << page_name << "\n";
    for (const auto& [param_name, value] : params) {
      std::cout << "  " << param_name 
		<< " = " << std::dec << value 
		<< " (0x" << std::hex << value << std::dec << ")\n";
    }
  }

  BOOST_CHECK_MESSAGE(chip_params.find("ALIGNER") != chip_params.end(),
                      "Page ALIGNER missing in decompiled settings");

  BOOST_CHECK_MESSAGE(chip_params.find("CLOCKSANDRESETS") != chip_params.end(),
                      "Page CLOCKSANDRESETS missing in decompiled settings");
}

BOOST_AUTO_TEST_CASE(full_lut_econd) {
  pflib::Compiler c = pflib::Compiler::get("econd_test");

  // map of register address and nbytes
  std::map<uint16_t, size_t> page_reg_byte_lut, expected;
  expected[0x0389] = 8;
  expected[0x03c5] = 3;
  expected[0x03c5] = 3;
  expected[0x0f2c] = 7;
  expected[0x0452] = 6;
  page_reg_byte_lut = c.build_register_byte_lut();

  for (const auto& [reg, nbytes] : page_reg_byte_lut) {
    std::cout << "0x"
              << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
	      << reg
              << " -> "
              << std::dec << nbytes << " bytes\n";
  }

  BOOST_CHECK_MESSAGE(
      page_reg_byte_lut == expected,
      "The generated register LUT does not match the expected LUT");
}

BOOST_AUTO_TEST_SUITE_END()
