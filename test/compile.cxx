#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "pflib/Compile.h"
#include "pflib/Exception.h"

BOOST_AUTO_TEST_SUITE( compile )

BOOST_AUTO_TEST_CASE( get_by_name ) {
  BOOST_CHECK_THROW(pflib::Compiler::get("sipm_v3"), pflib::Exception);
  BOOST_CHECK_NO_THROW(pflib::Compiler::get("sipm_rocv3"));
}

BOOST_AUTO_TEST_CASE( single_register_rocv3 ) {
  pflib::Compiler c = pflib::Compiler::get("sipm_rocv3");
  std::map<int,std::map<int,uint8_t>> registers, expected;
  c.compile("CH_45", "SEL_TRIG_TOA", 1, registers);
  expected[11][1] = 0b00000010;
  BOOST_CHECK_MESSAGE( registers == expected , "single-bit parameter setting" );
  c.compile("CH_45", "TRIM_TOA", 0b101010, registers);
  expected[11][1] = 0b10101010;
  BOOST_CHECK_MESSAGE( registers == expected , "multi-bit parameter setting" );
  c.compile("CH_45", "TRIM_TOA", 0, registers);
  expected[11][1] = 0b00000010;
  BOOST_CHECK_MESSAGE( registers == expected ,
      "multi-bit parameter setting not overwriting other parameters in register" );
  // current behavior: silently trim parameters that are too large
  c.compile("CH_45", "TRIM_TOA", 255, registers);
  expected[11][1] = 0b11111110;
  BOOST_CHECK_MESSAGE( registers == expected ,
      "silently-trim parameter if larger than number of bits available" );
}

BOOST_AUTO_TEST_CASE( one_param_multi_registers ) {
  pflib::Compiler c = pflib::Compiler::get("sipm_rocv3");
  std::map<int,std::map<int,uint8_t>> registers, expected;
  expected[89][24] = 0b00110011;
  expected[89][26] = 0b10110000;
  c.compile("DIGITALHALF_0", "BX_TRIGGER", 0b101100110011, registers);
  BOOST_CHECK_MESSAGE( registers == expected ,
      "set parameter across multiple registers" );
  c.compile("DIGITALHALF_0", "BX_OFFSET", 0b110111001100, registers);
  // register 24 value inherited from earlier
  expected[89][25] = 0b11001100;
  expected[89][26] = 0b10111101;
  BOOST_CHECK_MESSAGE( registers == expected ,
      "set parameter across multiple registers without overwriting other parameters in register" );
  // bx_trigger is only 12 bits, so shifting 1 by 13 ends up setting it back to zero
  c.compile("DIGITALHALF_0", "BX_TRIGGER", 1 << 13, registers);
  expected[89][24] = 0;
  expected[89][26] = 0b00001101;
  BOOST_CHECK_MESSAGE( registers == expected ,
      "silently trim parameter if larger than number of bits available");
}

BOOST_AUTO_TEST_SUITE_END()
