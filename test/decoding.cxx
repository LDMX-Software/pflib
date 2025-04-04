#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "pflib/Exception.h"
#include "pflib/packing/DAQLinkFrame.h"
#include "pflib/packing/Sample.h"

BOOST_AUTO_TEST_SUITE(decoding)

BOOST_AUTO_TEST_SUITE(sample)

BOOST_AUTO_TEST_CASE(zero_word) {
  pflib::packing::Sample s;
  s.word = 0;
  BOOST_CHECK(s.Tc() == false);
  BOOST_CHECK(s.Tp() == false);
  BOOST_CHECK(s.adc() == 0);
  BOOST_CHECK(s.tot() == -1);
  BOOST_CHECK(s.adc_tm1() == 0);
  BOOST_CHECK(s.toa() == 0);
}

BOOST_AUTO_TEST_CASE(simple_adc) {
  pflib::packing::Sample s;
  s.word = 0b00000000000100000000100000000011;
  BOOST_CHECK(s.Tc() == false);
  BOOST_CHECK(s.Tp() == false);
  BOOST_CHECK(s.adc() == 2);
  BOOST_CHECK(s.tot() == -1);
  BOOST_CHECK(s.adc_tm1() == 1);
  BOOST_CHECK(s.toa() == 3);
}

BOOST_AUTO_TEST_CASE(tot_output) {
  pflib::packing::Sample s;
  s.word = 0b11000000000100000000100000000011;
  BOOST_CHECK(s.Tc() == true);
  BOOST_CHECK(s.Tp() == true);
  BOOST_CHECK(s.adc() == -1);
  BOOST_CHECK(s.tot() == 2);
  BOOST_CHECK(s.adc_tm1() == 1);
  BOOST_CHECK(s.toa() == 3);
}

BOOST_AUTO_TEST_CASE(tot_busy) {
  pflib::packing::Sample s;
  s.word = 0b01000000000100000000100000000011;
  BOOST_CHECK(s.Tc() == false);
  BOOST_CHECK(s.Tp() == true);
  BOOST_CHECK(s.adc() == 2);
  BOOST_CHECK(s.tot() == -1);
  BOOST_CHECK(s.adc_tm1() == 1);
  BOOST_CHECK(s.toa() == 3);
}

BOOST_AUTO_TEST_CASE(characterization) {
  pflib::packing::Sample s;
  s.word = 0b10000000000100000000100000000011;
  BOOST_CHECK(s.Tc() == true);
  BOOST_CHECK(s.Tp() == false);
  BOOST_CHECK(s.adc() == 1);
  BOOST_CHECK(s.tot() == 2);
  BOOST_CHECK(s.adc_tm1() == -1);
  BOOST_CHECK(s.toa() == 3);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(daq_link_frame)

std::vector<uint32_t> gen_test_frame() {
  std::vector<uint32_t> test_frame = {
      0xf00c26a2,  // daq header
      0x00022802,  // common mode
  };
  std::size_t i_ch{0};
  for (; i_ch < 18; i_ch++) {
    test_frame.push_back(i_ch);
  }
  // mark calib channel as TOT busy
  test_frame.push_back(0b01000000000000000000000000000000);
  for (; i_ch < 36; i_ch++) {
    test_frame.push_back(i_ch);
  }
  // not testing CRC, just put in dummy word
  test_frame.push_back(0x0f0f0f0f);
  return test_frame;
}

std::vector<uint32_t> test_frame = gen_test_frame();

BOOST_AUTO_TEST_CASE(foo) {
  pflib::packing::DAQLinkFrame f;
  BOOST_REQUIRE_NO_THROW(f.from(test_frame));
  BOOST_CHECK(f.bx == 12);
  BOOST_CHECK(f.event == 9);
  BOOST_CHECK(f.orbit == 5);
  BOOST_CHECK(f.second_quarter_err == false);
  BOOST_CHECK(f.first_quarter_err == true);
  BOOST_CHECK(f.counter_err == false);
  BOOST_CHECK(f.adc_cm1 == 2);
  BOOST_CHECK(f.adc_cm0 == 138);

  BOOST_CHECK(f.calib.Tc() == false);
  BOOST_CHECK(f.calib.Tp() == true);
  BOOST_CHECK(f.calib.adc() == 0);
  BOOST_CHECK(f.calib.tot() == -1);
  BOOST_CHECK(f.calib.adc_tm1() == 0);
  BOOST_CHECK(f.calib.toa() == 0);

  for (std::size_t i_ch{0}; i_ch < f.channels.size(); i_ch++) {
    const auto& ch{f.channels[i_ch]};
    BOOST_CHECK(ch.Tc() == false);
    BOOST_CHECK(ch.Tp() == false);
    BOOST_CHECK(ch.adc() == 0);
    BOOST_CHECK(ch.tot() == -1);
    BOOST_CHECK(ch.adc_tm1() == 0);
    BOOST_CHECK(ch.toa() == i_ch);
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
