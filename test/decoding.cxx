#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "pflib/Exception.h"
#include "pflib/packing/DAQLinkFrame.h"
#include "pflib/packing/Mask.h"
#include "pflib/packing/Hex.h"
#include "pflib/packing/Sample.h"
#include "pflib/packing/TriggerLinkFrame.h"
#include "pflib/packing/ECONDEventPacket.h"

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

BOOST_AUTO_TEST_CASE(from_unpacked_zs) {
  pflib::packing::Sample s;
  // fully zero-suppressed sample produces zero word
  s.from_unpacked(false, false, -1, 0, -1);
  BOOST_CHECK_EQUAL(s.word, 0);
}

BOOST_AUTO_TEST_CASE(from_unpacked_adc) {
  pflib::packing::Sample s;
  s.from_unpacked(false, false, -1, 42, -1);
  BOOST_CHECK(s.Tc() == false);
  BOOST_CHECK(s.Tp() == false);
  BOOST_CHECK(s.adc() == 42);
  BOOST_CHECK(s.tot() == -1);
  BOOST_CHECK(s.toa() == 0);
  BOOST_CHECK(s.adc_tm1() == 0);
}

BOOST_AUTO_TEST_CASE(from_unpacked_adc_toa) {
  pflib::packing::Sample s;
  s.from_unpacked(false, false, 21, 42, 12);
  BOOST_CHECK(s.Tc() == false);
  BOOST_CHECK(s.Tp() == false);
  BOOST_CHECK(s.adc() == 42);
  BOOST_CHECK(s.tot() == -1);
  BOOST_CHECK(s.toa() == 12);
  BOOST_CHECK(s.adc_tm1() == 21);
}

BOOST_AUTO_TEST_CASE(from_unpacked_tot) {
  pflib::packing::Sample s;
  s.from_unpacked(true, true, 21, 33, 12);
  BOOST_CHECK(s.Tc() == true);
  BOOST_CHECK(s.Tp() == true);
  BOOST_CHECK(s.adc() == -1);
  BOOST_CHECK(s.tot() == 33);
  BOOST_CHECK(s.toa() == 12);
  BOOST_CHECK(s.adc_tm1() == 21);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(daq_link_frame)

BOOST_AUTO_TEST_CASE(simulated_frame) {
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
  BOOST_CHECK_EQUAL(
      decomp,
      pflib::packing::TriggerLinkFrame::compressed_to_linearized(compressed));
}

BOOST_AUTO_TEST_CASE(decompress_zero) {
  BOOST_CHECK_EQUAL(
      0, pflib::packing::TriggerLinkFrame::compressed_to_linearized(0));
}

BOOST_AUTO_TEST_CASE(decompress_small) {
  BOOST_CHECK_EQUAL(
      5, pflib::packing::TriggerLinkFrame::compressed_to_linearized(5));
}

BOOST_AUTO_TEST_CASE(decompress_large) {
  uint8_t compressed = 0b1111011;
  uint32_t decomp = 0b101100000000000000;
  BOOST_CHECK_EQUAL(
      decomp,
      pflib::packing::TriggerLinkFrame::compressed_to_linearized(compressed));
}

BOOST_AUTO_TEST_CASE(full_frame) {
  std::vector<uint32_t> frame = {
      0x300000f0,  // sw header
      0xafe00000,  // i_sum=0 is full at i_bx=-1
      0xa01fc000,  // i_sum=1 is full at i_bx=0
      0xa0003f80,  // i_sum=2 is full at i_bx=1
      0xa000007f   // i_sum=3 is full at i_bx=2
  };
  pflib::packing::TriggerLinkFrame tlf(frame);
  BOOST_CHECK_EQUAL(tlf.i_link, 0xf0);
  for (int i_bx{-1}; i_bx < 3; i_bx++) {
    for (int i_sum{0}; i_sum < 4; i_sum++) {
      BOOST_TEST_INFO("checking compressed sum " << i_sum << " in BX " << i_bx);
      BOOST_CHECK_EQUAL(tlf.compressed_sum(i_sum, i_bx),
                        (i_sum - 1 == i_bx) ? 0x7f : 0);
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(econd)

BOOST_AUTO_TEST_CASE(real_full_frame) {
  /**
   * This single event packet was collected by Cristina at UVA
   * from an ECON-D with
   * - no ZS but does not appear to be in pass through mode
   * - 6 eRx -> 6 DAQ links connected
   * - 2 eTx -> 2 Trigger links (ignored?)
   */
  std::vector<uint32_t> frame = {
    0xf32d5010, 0xde92c07c, // two-word event packet header
    0xe0308fff, 0xffffffff, // link sub-packet header with channel map
    0x01ec7f03, 0xfd07015c, 0x67026c99, 0x0fffff0f, 0xffff0fff, 0xff0fffff, 0x017c5f0f,
    0xffff0475, 0x1b0fffff, 0x02fcbb01, 0xb46d01cc, 0x72032cd1, 0x0254a100, 0xd43506ec,
    0x0001785d, 0x0fffff03, 0x4cd10fff, 0xff0390f4, 0x0fffff0f, 0xffff0966, 0x630fffff,
    0x00741f01, 0xbc72017c, 0x5b014457, 0x017c6a0f, 0xffff0fff, 0xff0a3280, 0x0a5a9c00,
    0xe0328edf, 0xffffffff, // link sub-packet header with channel map
    0x01484f02, 0x10780220, 0x7a023c8c, 0x027ca601, 0xcc7a027c, 0x9901a869, 0x01906301,
    0x6c5501ac, 0x7402549f, 0x0264a301, 0xa85f024c, 0x99019c6f, 0x02fcbf01, 0x745d04dc,
    0x00024892, 0x026ca102, 0x7c9d02f0, 0xc5021c86, 0x01545a01, 0xec7b0250, 0x8e02ccad,
    0x019c6602, 0x2c8f028c, 0xad0fffff, 0x0fffff0f, 0xffff063d, 0x9902dcc6, 0x0fffff00,
    0xe03d8bff, 0xffffffff, // link sub-packet header with channel map
    0x0188630f, 0xffff0fff, 0xff01a86a, 0x0d57500a, 0x9aa60fff, 0xfe0fffff, 0x01bc6e0f,
    0xffff0e33, 0x940fffff, 0x02649301, 0xc8840214, 0x81023c99, 0x0eefc101, 0x7c620198,
    0x00018c64, 0x0fffff03, 0x44d20fff, 0xff0b12c2, 0x0fffff0f, 0xffff0fff, 0xff0fffff,
    0x01585501, 0xd48301d4, 0x7d01b479, 0x01b06a0f, 0xffff0aae, 0xa00fffff, 0x0fffff00,
    0xe0310cbf, 0xffffffff, // link sub-packet header with channel map
    0x02047402, 0x348d0208, 0x820aaea1, 0x02449702, 0x3895029c, 0xa801fc7f, 0x01b06d01,
    0xb4750264, 0x9801ec75, 0x018c6702, 0x7ca10234, 0x96016867, 0x01cc7d01, 0x986601ac,
    0x00015457, 0x02148101, 0xf07f02f8, 0xbb02147b, 0x01b06501, 0xfc7502e4, 0xb3021884,
    0x016c5a0f, 0xffff0f47, 0xe50fffff, 0x0fffff0f, 0xffff0fff, 0xff0fffff, 0x0fffff00,
    0xe0378e7f, 0xffffffff, // link sub-packet header with channel map
    0x01dc7d01, 0xb46c0268, 0x9701f879, 0x0fffff07, 0xa1ee03fd, 0x050fffff, 0x012c4b0f,
    0xbbf602b0, 0xad0a6e9c, 0x02589a02, 0x44970134, 0x5902ccad, 0x0314c600, 0xf83d067c,
    0x00011c47, 0x01fc7f07, 0xb1eb0b22, 0xc801d473, 0x0a2a930d, 0xa3660274, 0x9e0fffff,
    0x00fc3f01, 0x7c5b0274, 0xa5013c53, 0x01544f0f, 0xffff01cc, 0x72013c51, 0x095a5d00,
    0xe02889df, 0xffffffff, // link sub-packet header with channel map
    0x02fcbf01, 0x785402a0, 0x97021885, 0x01544f01, 0xc87102dc, 0xaf01ec7f, 0x01d47601,
    0x5c5d01f0, 0x7901f487, 0x02ecbf02, 0xdcb901c8, 0x6e02fccd, 0x02b8ae01, 0x88620694,
    0x0001cc73, 0x02348900, 0x040109ba, 0x6a01745d, 0x020c8a01, 0xfc75025c, 0x9f01d46f,
    0x01bc6f0f, 0xffff019c, 0x690fffff, 0x0fffff0f, 0xffff0a4a, 0x96023c87, 0x0fffff00,
    0x1bb1292f // CRC for sub-packets
               // no IDLE word?
  };

  //pflib::logging::set(pflib::logging::level::trace);
  pflib::packing::ECONDEventPacket ep{6};
  ep.from(frame);

  BOOST_CHECK_MESSAGE(
      (not ep.corruption[0]),
      "9b Header Marker same as Spec");
  BOOST_CHECK_MESSAGE(
      (not ep.corruption[1]),
      "Payload length in header equal to frame size minus 2");
  BOOST_CHECK_MESSAGE(
      (not ep.corruption[2]),
      "8b Header CRC matches transmitted value.");
  BOOST_CHECK_MESSAGE(
      (not ep.corruption[3]),
      "32b Link Subpacket CRC matches transmitted value.");

  /**
   * In this no-signal single-event without zero suppression,
   * all of the channels are in ADC mode with zero TOA.
   */
  for (const auto& link: ep.links) {
    BOOST_CHECK_MESSAGE(
        (not link.corruption[0]),
        "Link Stat checks all good");
    BOOST_CHECK_EQUAL(link.calib.toa(), 0);
    BOOST_CHECK_EQUAL(link.calib.tot(), -1);
    // all of the calib channels have 0 ADC and non-zero ADC(t-1)
    // maybe that's on purpose? Idk
    BOOST_CHECK_MESSAGE(
        link.calib.adc() == 0,
        "Calib channel ADC " << link.calib.adc() << " is zero");
    BOOST_CHECK_MESSAGE(
        (link.calib.adc_tm1() > 0 and link.calib.adc_tm1() < 1024),
        "Calib channel ADC(t-1) " << link.calib.adc_tm1() << " out of range");
    for (const auto& ch: link.channels) {
      BOOST_CHECK_EQUAL(ch.toa(), 0);
      BOOST_CHECK_EQUAL(ch.tot(), -1);
      BOOST_CHECK_MESSAGE((ch.adc() > 0 and ch.adc() < 1024), "channel ADC out of range");
      BOOST_CHECK_MESSAGE((ch.adc_tm1() > 0 and ch.adc_tm1() < 1024), "channel ADC(t-1) out of range");
    }
  }
  /*
  std::ofstream of("eg-econd-nozs.csv");
  of << pflib::packing::ECONDEventPacket::to_csv_header << '\n';
  ep.to_csv(of);
  */
}

BOOST_AUTO_TEST_CASE(econd_spec_example_fig33) {
  using pflib::packing::mask;
  /**
   * Dropping the eRx 2-N sub-packet rows, this is a direct copy of the example
   * in Figure 33 with some arbitrarily chosen channels
   */
  unsigned int bx{420},l1a{12},orb{3};
  unsigned int l0_cm0{576}, l0_cm1{999},
      l1_cm0{3}, l1_cm1{58};
  unsigned int ch0_adctm1{1}, ch0_adc{2},
      ch1_adc{(1<<6)+1},
      ch2_adctm1{122}, ch2_adc{498},
      ch3_adc{537}, ch3_toa{111},
      ch4_adc{0b0101000110}, ch4_tot{874}, ch4_toa{777},
      ch5_adctm1{0b1001001011}, ch5_tot{875}, ch5_toa{778},
      ch6_adctm1{0b0001100001}, ch6_adc{876}, ch6_toa{8};
  std::vector<uint32_t> frame;
  frame.resize(12);
  // two word event packet header
  frame[0] = (0xaau<<24)+(10<<14)+(0<<13)+(1u<<12)+(1u<<7);
  frame[1] = (bx<<20)+(l1a<<14)+(orb<<11)+0b11000110;
  // two word link sub-packet header with channel map
  // 7 channels readout: 31, 29, 26, 18, calib, 10, 0
  frame[2] = (0b111u<<29)+(l0_cm0 << 15)+(l0_cm1 << 5)+0b00001;
  frame[3] = 0b01001000000011000000010000000001u;
  // 7 channels concentrated into 6 words
  frame[4] = (ch0_adctm1 << 18)+(ch0_adc << 8)+(0b0001 << 4)+(ch1_adc >> 6);
  frame[5] = ((ch1_adc & mask<6>) << 26) + (0b0010 << 20) + (ch2_adctm1 << 10) + ch2_adc;
  frame[6] = (0b0011u << 28) + (ch3_adc << 18) + (ch3_toa << 8) + (0b10 << 6) + (ch4_adc >> 4);
  frame[7] = ((ch4_adc & mask<4>) << 28) + (ch4_tot << 18) + (ch4_toa << 8) + (0b11 << 6) + (ch5_adctm1 >> 4);
  frame[8] = ((ch5_adctm1 & mask<4>) << 28) + (ch5_tot << 18) + (ch5_toa << 8) + (0b01 << 6) + (ch6_adctm1 >> 4);
  frame[9] = ((ch6_adctm1 & mask<4>) << 28) + (ch6_adc << 18) + (ch6_toa << 8); 
  // single word for empty link sub-packet
  frame[10] = (0b111u<<29)+(1u << 25)+(l1_cm0 << 15)+(l1_cm1 << 5)+0b10001u;
  // CRC
  frame[11] = 0x8a89abb6;

  //pflib::logging::set(pflib::logging::level::trace);
  pflib::packing::ECONDEventPacket ep{2};
  ep.from(frame);

  BOOST_CHECK_MESSAGE(not ep.corruption[0], "test frame has bad header code");
  BOOST_CHECK_MESSAGE(not ep.corruption[1], "test frame and decoding have mismatched lengths");
  BOOST_CHECK_MESSAGE(not ep.corruption[2], "8-bit header CRC calculation done in software, needs to be updated once aligned with real hardware implementation");
  BOOST_CHECK_MESSAGE(not ep.corruption[3], "Subpacket CRC calculation done in software, needs to be updated once aligned with real hardware implementation");

  BOOST_TEST_INFO("Checking transmitted channel 0 (should be CH 0)");
  BOOST_CHECK_EQUAL(ch0_adctm1, ep.links[0].channels[0].adc_tm1());
  BOOST_CHECK_EQUAL(ch0_adc, ep.links[0].channels[0].adc());
  BOOST_CHECK_EQUAL(0, ep.links[0].channels[0].toa());
  BOOST_CHECK_EQUAL(-1, ep.links[0].channels[0].tot());

  BOOST_TEST_INFO("Checking transmitted channel 1 (should be CH 10)");
  BOOST_CHECK_EQUAL(0, ep.links[0].channels[10].adc_tm1());
  BOOST_CHECK_EQUAL(ch1_adc, ep.links[0].channels[10].adc());
  BOOST_CHECK_EQUAL(0, ep.links[0].channels[10].toa());
  BOOST_CHECK_EQUAL(-1, ep.links[0].channels[10].tot());

  BOOST_TEST_INFO("Checking transmitted channel 2 (should be CALIB)");
  BOOST_CHECK_EQUAL(ch2_adctm1, ep.links[0].calib.adc_tm1());
  BOOST_CHECK_EQUAL(ch2_adc, ep.links[0].calib.adc());
  BOOST_CHECK_EQUAL(0, ep.links[0].calib.toa());
  BOOST_CHECK_EQUAL(-1, ep.links[0].calib.tot());

  BOOST_TEST_INFO("Checking transmitted channel 3 (should be CH 18)");
  BOOST_CHECK_EQUAL(0, ep.links[0].channels[18].adc_tm1());
  BOOST_CHECK_EQUAL(ch3_adc, ep.links[0].channels[18].adc());
  BOOST_CHECK_EQUAL(ch3_toa, ep.links[0].channels[18].toa());
  BOOST_CHECK_EQUAL(-1, ep.links[0].channels[18].tot());

  BOOST_TEST_INFO("Checking transmitted channel 4 (should be CH 26)");
  BOOST_CHECK_EQUAL(-1, ep.links[0].channels[26].adc_tm1());
  BOOST_CHECK_EQUAL(ch4_adc, ep.links[0].channels[26].adc());
  BOOST_CHECK_EQUAL(ch4_toa, ep.links[0].channels[26].toa());
  BOOST_CHECK_EQUAL(ch4_tot, ep.links[0].channels[26].tot());

  BOOST_TEST_INFO("Checking transmitted channel 5 (should be CH 29)");
  BOOST_CHECK_EQUAL(ch5_adctm1, ep.links[0].channels[29].adc_tm1());
  BOOST_CHECK_EQUAL(-1, ep.links[0].channels[29].adc());
  BOOST_CHECK_EQUAL(ch5_toa, ep.links[0].channels[29].toa());
  BOOST_CHECK_EQUAL(ch5_tot, ep.links[0].channels[29].tot());

  BOOST_TEST_INFO("Checking transmitted channel 6 (should be CH 31)");
  BOOST_CHECK_EQUAL(ch6_adctm1, ep.links[0].channels[31].adc_tm1());
  BOOST_CHECK_EQUAL(ch6_adc, ep.links[0].channels[31].adc());
  BOOST_CHECK_EQUAL(ch6_toa, ep.links[0].channels[31].toa());
  BOOST_CHECK_EQUAL(-1, ep.links[0].channels[31].tot());

  for (std::size_t i_ch{0}; i_ch < 36; i_ch++) {
    if (i_ch == 0 or i_ch == 10 or i_ch == 18 or i_ch == 26 or i_ch == 29 or i_ch == 31) {
      continue;
    }
    BOOST_TEST_INFO("Checking that CH " << i_ch << " is empty");
    BOOST_CHECK_EQUAL(0, ep.links[0].channels[i_ch].word);
  }
}

BOOST_AUTO_TEST_CASE(one_link_passthrough) {
  using pflib::packing::mask;

  unsigned int bx{222},l1a{42},orb{2};
  std::vector<uint32_t> frame(test_frame.size()+2);
  // econd header words
  frame[0] = (0xaa << 24) + ((39*1 + 1) << 14) + (1 << 13);
  frame[1] = (bx<<20)+(l1a<<14)+(orb<<11)+0b11000110;
  // copy over test frame
  std::copy(test_frame.begin(), test_frame.end(), frame.begin()+2);
  // replace link header words
  frame[2] = (0b111 << 29) + (((frame[3] >> 10) & mask<10>) << 15) + ((frame[3] & mask<10>) << 5) + 0b11111;
  frame[3] = 0xffffffff;
  frame[41] = 1;

  pflib::packing::ECONDEventPacket ep{1};
  ep.from(frame);

  const auto& f{ep.links[0]};

  BOOST_CHECK(f.bx == 222);
  BOOST_CHECK(f.event == 42);
  BOOST_CHECK(f.orbit == 2);
  BOOST_CHECK(f.corruption[0] == false);
  BOOST_CHECK(f.corruption[1] == false);
  BOOST_CHECK(f.corruption[2] == false);
  BOOST_CHECK(f.corruption[3] == false);
  BOOST_CHECK(f.corruption[4] == false);
  BOOST_CHECK(f.corruption[5] == false);
  BOOST_CHECK_EQUAL(f.adc_cm1,  2);
  BOOST_CHECK_EQUAL(f.adc_cm0, 138);

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
