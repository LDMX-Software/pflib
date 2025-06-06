#define BOOST_TEST_DYN_LINK
#include "pflib/ROC.h"
#include "hgcroc_connection.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(test_parameters)

BOOST_AUTO_TEST_CASE(eg) {
  auto roc{hgcroc_connection::tgt->hcal().roc(0)};
  // CH_40 is Page 6 and InputDac is Register 0
  // for SiPM_rocv3b
  auto original = roc.readPage(6, 1)[0];
  {
    auto tp = roc.testParameters()
      .add("CH_40", "INPUTDAC", 0x2a)
      .apply();
    auto test = roc.readPage(6, 1)[0];
    BOOST_CHECK_MESSAGE(test == 0x2a, "test parameter setting applied");
  }
  auto test = roc.readPage(6, 1)[0];
  BOOST_CHECK_MESSAGE(test == original, "parameter reset to original value");
}

BOOST_AUTO_TEST_CASE(temp_zero) {
  auto roc{hgcroc_connection::tgt->hcal().roc(0)};
  // CH_40 is Page 6 and InputDac is Register 0
  // for sipm_rocv3b
  auto original = roc.readPage(6, 1)[0];
  roc.setValue(6, 0, 0x2a);
  {
    auto tp = roc.testParameters()
      .add("CH_40", "INPUTDAC", 0)
      .apply();
    auto test = roc.readPage(6, 1)[0];
    BOOST_CHECK_MESSAGE(test == 0, "test parameter set to zero");
  }
  auto test = roc.readPage(6, 1)[0];
  BOOST_CHECK_MESSAGE(test == 0x2a, "parameter reset to original non-zero value");
  roc.setValue(6, 0, original);
}

BOOST_AUTO_TEST_SUITE_END()
