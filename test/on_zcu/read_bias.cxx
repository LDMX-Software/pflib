#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "hgcroc_connection.h"
#include "pflib/Hcal.h"

BOOST_AUTO_TEST_SUITE(read_bias)

void read_biases() {
  pflib::Bias bias =
      dynamic_cast<pflib::Hcal*>(hgcroc_connection::tgt.get())->bias(0);
  for (int i = 0; i < 16; i++) {
    bias.readLED(i);
    bias.readSiPM(i);
  }
}

BOOST_AUTO_TEST_CASE(read_bias) { BOOST_REQUIRE_NO_THROW(read_biases()); }

BOOST_AUTO_TEST_SUITE_END()
