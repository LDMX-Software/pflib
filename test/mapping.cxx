#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "pflib/Exception.h"
#include "pflib/packing/SingleECONDRocErxMapping.h"
#include "pflib/HcalBackplane.h"

BOOST_AUTO_TEST_SUITE(mapping)

BOOST_AUTO_TEST_CASE(null_shuffle) {
  pflib::packing::SingleECONDRocErxMapping mapping({{0,1}}, {0});

  BOOST_CHECK_EQUAL(mapping.toErx(0,0), 0);
  BOOST_CHECK_EQUAL(mapping.toErx(0,1), 1);

  {
    auto [i_roc, half] = mapping.toROCHalf(0);
    BOOST_CHECK_EQUAL(i_roc, 0);
    BOOST_CHECK_EQUAL(half, 0);
  }

  {
    auto [i_roc, half] = mapping.toROCHalf(1);
    BOOST_CHECK_EQUAL(i_roc, 0);
    BOOST_CHECK_EQUAL(half, 1);
  }

  {
    auto [i_roc, channel] = mapping.toROCChannel(0, 32);
    BOOST_CHECK_EQUAL(i_roc, 0);
    BOOST_CHECK_EQUAL(channel, 32);
  }

  {
    auto [i_roc, channel] = mapping.toROCChannel(1, 32);
    BOOST_CHECK_EQUAL(i_roc, 0);
    BOOST_CHECK_EQUAL(channel, 68);
  }

  {
    auto [i_erx, channel] = mapping.toErxChannel(0, 32);
    BOOST_CHECK_EQUAL(i_erx, 0);
    BOOST_CHECK_EQUAL(channel, 32);
  }

  {
    auto [i_erx, channel] = mapping.toErxChannel(0, 60);
    BOOST_CHECK_EQUAL(i_erx, 1);
    BOOST_CHECK_EQUAL(channel, 24);
  }
}

BOOST_AUTO_TEST_SUITE(hcal_backplane)

BOOST_AUTO_TEST_CASE(single_hgcroc0) {
  pflib::packing::SingleECONDRocErxMapping mapping(pflib::HcalBackplane::ROC_ERX_MAPPING, {0});

  BOOST_CHECK_EQUAL(mapping.toErx(0,0), 1);
  BOOST_CHECK_EQUAL(mapping.toErx(0,1), 0);

  {
    auto [i_roc, half] = mapping.toROCHalf(0);
    BOOST_CHECK_EQUAL(i_roc, 0);
    BOOST_CHECK_EQUAL(half, 1);
  }

  {
    auto [i_roc, half] = mapping.toROCHalf(1);
    BOOST_CHECK_EQUAL(i_roc, 0);
    BOOST_CHECK_EQUAL(half, 0);
  }

  {
    auto [i_roc, channel] = mapping.toROCChannel(0, 32);
    BOOST_CHECK_EQUAL(i_roc, 0);
    BOOST_CHECK_EQUAL(channel, 68);
  }

  {
    auto [i_roc, channel] = mapping.toROCChannel(1, 32);
    BOOST_CHECK_EQUAL(i_roc, 0);
    BOOST_CHECK_EQUAL(channel, 32);
  }

  {
    auto [i_erx, channel] = mapping.toErxChannel(0, 32);
    BOOST_CHECK_EQUAL(i_erx, 1);
    BOOST_CHECK_EQUAL(channel, 32);
  }

  {
    auto [i_erx, channel] = mapping.toErxChannel(0, 60);
    BOOST_CHECK_EQUAL(i_erx, 0);
    BOOST_CHECK_EQUAL(channel, 24);
  }
}

BOOST_AUTO_TEST_CASE(single_hgcroc1) {
  // HGCROC1 is lined up in half-order like the no-shuffle
  pflib::packing::SingleECONDRocErxMapping mapping(pflib::HcalBackplane::ROC_ERX_MAPPING, {1});

  BOOST_CHECK_EQUAL(mapping.toErx(1,0), 0);
  BOOST_CHECK_EQUAL(mapping.toErx(1,1), 1);

  {
    auto [i_roc, half] = mapping.toROCHalf(0);
    BOOST_CHECK_EQUAL(i_roc, 1);
    BOOST_CHECK_EQUAL(half, 0);
  }

  {
    auto [i_roc, half] = mapping.toROCHalf(1);
    BOOST_CHECK_EQUAL(i_roc, 1);
    BOOST_CHECK_EQUAL(half, 1);
  }

  {
    auto [i_roc, channel] = mapping.toROCChannel(0, 32);
    BOOST_CHECK_EQUAL(i_roc, 1);
    BOOST_CHECK_EQUAL(channel, 32);
  }

  {
    auto [i_roc, channel] = mapping.toROCChannel(1, 32);
    BOOST_CHECK_EQUAL(i_roc, 1);
    BOOST_CHECK_EQUAL(channel, 68);
  }

  {
    auto [i_erx, channel] = mapping.toErxChannel(1, 32);
    BOOST_CHECK_EQUAL(i_erx, 0);
    BOOST_CHECK_EQUAL(channel, 32);
  }

  {
    auto [i_erx, channel] = mapping.toErxChannel(1, 60);
    BOOST_CHECK_EQUAL(i_erx, 1);
    BOOST_CHECK_EQUAL(channel, 24);
  }
}

BOOST_AUTO_TEST_CASE(two_hgcroc12) {
  // HGCROC1 has eRx after HGCROC2
  // I'm not testing the to*Channel functions here since
  // I trust the above faithfully checks that for me
  pflib::packing::SingleECONDRocErxMapping mapping(pflib::HcalBackplane::ROC_ERX_MAPPING, {1,2});

  BOOST_CHECK_EQUAL(mapping.toErx(1,0), 2);
  BOOST_CHECK_EQUAL(mapping.toErx(1,1), 3);
  BOOST_CHECK_EQUAL(mapping.toErx(2,0), 0);
  BOOST_CHECK_EQUAL(mapping.toErx(2,1), 1);

  {
    auto [i_roc, half] = mapping.toROCHalf(0);
    BOOST_CHECK_EQUAL(i_roc, 2);
    BOOST_CHECK_EQUAL(half, 0);
  }

  {
    auto [i_roc, half] = mapping.toROCHalf(1);
    BOOST_CHECK_EQUAL(i_roc, 2);
    BOOST_CHECK_EQUAL(half, 1);
  }

  {
    auto [i_roc, half] = mapping.toROCHalf(2);
    BOOST_CHECK_EQUAL(i_roc, 1);
    BOOST_CHECK_EQUAL(half, 0);
  }

  {
    auto [i_roc, half] = mapping.toROCHalf(3);
    BOOST_CHECK_EQUAL(i_roc, 1);
    BOOST_CHECK_EQUAL(half, 1);
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
