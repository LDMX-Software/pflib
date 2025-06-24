#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "pflib/Exception.h"
#include "pflib/packing/DAQLinkFrame.h"
#include "pflib/packing/Sample.h"
#include "pflib/packing/Mask.h"
#include "pflib/packing/TriggerLinkFrame.h"

BOOST_AUTO_TEST_SUITE(decoding)

BOOST_AUTO_TEST_SUITE(mask)

BOOST_AUTO_TEST_CASE(ten_bits) {
  BOOST_CHECK_EQUAL(pflib::packing::mask<10>, 0x3ff);
}

BOOST_AUTO_TEST_SUITE_END()

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

BOOST_AUTO_TEST_CASE(high_bits) {
  pflib::packing::Sample s;
  s.word = 0b00100000000110000000101000000011;
  BOOST_CHECK(s.Tc() == false);
  BOOST_CHECK(s.Tp() == false);
  BOOST_CHECK(s.adc() == 514);
  BOOST_CHECK(s.tot() == -1);
  BOOST_CHECK(s.adc_tm1() == 513);
  BOOST_CHECK(s.toa() == 515);
}

BOOST_AUTO_TEST_CASE(real_word_1) {
  pflib::packing::Sample s;
  s.word = 0x03012c02;
  BOOST_CHECK(s.Tc() == false);
  BOOST_CHECK(s.Tp() == false);
  BOOST_CHECK(s.adc() == 75);
  BOOST_CHECK(s.tot() == -1);
  BOOST_CHECK(s.adc_tm1() == 48);
  BOOST_CHECK(s.toa() == 2);
}

BOOST_AUTO_TEST_CASE(real_word_2) {
  pflib::packing::Sample s;
  s.word = 0x08208a02;
  BOOST_CHECK(s.Tc() == false);
  BOOST_CHECK(s.Tp() == false);
  BOOST_CHECK_EQUAL(s.adc(), 34);
  BOOST_CHECK_EQUAL(s.tot(), -1);
  BOOST_CHECK_EQUAL(s.adc_tm1(), 130);
  BOOST_CHECK_EQUAL(s.toa(), 514);
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
      0xf00c26a5,  // daq header
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
  // CRC word calculated by our own code
  // just to quiet the testing
  test_frame.push_back(0xe2378cb3);
  return test_frame;
}

std::vector<uint32_t> test_frame = gen_test_frame();

BOOST_AUTO_TEST_CASE(foo) {
  pflib::packing::DAQLinkFrame f;
  BOOST_REQUIRE_NO_THROW(f.from(test_frame));
  BOOST_CHECK(f.bx == 12);
  BOOST_CHECK(f.event == 9);
  BOOST_CHECK(f.orbit == 5);
  BOOST_CHECK(f.corruption[0] == false);
  BOOST_CHECK(f.corruption[1] == false);
  BOOST_CHECK(f.corruption[2] == false);
  BOOST_CHECK(f.corruption[3] == true);
  BOOST_CHECK(f.corruption[4] == false);
  BOOST_CHECK(f.corruption[5] == false);
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

BOOST_AUTO_TEST_SUITE(trigger)

BOOST_AUTO_TEST_CASE(example_decompression) {
  uint8_t compressed = 0b0100111;
  uint32_t decomp = 0b1111000;
  BOOST_CHECK_EQUAL(decomp, pflib::packing::TriggerLinkFrame::compressed_to_linearized(compressed));
}

BOOST_AUTO_TEST_CASE(decompress_zero) {
  BOOST_CHECK_EQUAL(0, pflib::packing::TriggerLinkFrame::compressed_to_linearized(0));
}

BOOST_AUTO_TEST_CASE(decompress_small) {
  BOOST_CHECK_EQUAL(5, pflib::packing::TriggerLinkFrame::compressed_to_linearized(5));
}

BOOST_AUTO_TEST_CASE(decompress_large) {
  uint8_t compressed = 0b1111011;
  uint32_t decomp = 0b101100000000000000;
  BOOST_CHECK_EQUAL(decomp, pflib::packing::TriggerLinkFrame::compressed_to_linearized(compressed));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
