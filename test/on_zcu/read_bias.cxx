#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "hgcroc_connection.h"
#include "pflib/HcalBackplane.h"

BOOST_AUTO_TEST_SUITE(read_bias)

void read_biases() {
  auto hcal = dynamic_cast<pflib::HcalBackplane*>(hgcroc_connection::tgt.get());
  if (not hcal) return;
  pflib::Bias bias = hcal->bias(0);
  for (int i = 0; i < 16; i++) {
    bias.readLED(i);
    bias.readSiPM(i);
  }
}

BOOST_AUTO_TEST_CASE(read_bias) { BOOST_REQUIRE_NO_THROW(read_biases()); }

BOOST_AUTO_TEST_SUITE_END()
