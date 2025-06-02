#define BOOST_TEST_DYN_LINK
#include "hgcroc_connection.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(elinks_aligned)

BOOST_AUTO_TEST_CASE(check_daq_idleframe) {
  /**
   * We temporarily set the idleframe to something
   * that would allow us to detect a nibble shuffle
   */
  auto roc{hgcroc_connection::tgt->hcal().roc(0)};
  auto tp = roc.testParameters()
    .add("DIGITALHALF_0", "IDLEFRAME", 0x1234567)
    .add("DIGITALHALF_1", "IDLEFRAME", 0x1234567)
    .apply();
  auto& elinks{hgcroc_connection::tgt->hcal().elinks()};
  for (std::size_t i_link{0}; i_link < 2; i_link++) {
    auto spy{elinks.spy(i_link)};
    for (auto word : spy) {
      BOOST_CHECK_EQUAL(word, 0xa1234567);
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()
