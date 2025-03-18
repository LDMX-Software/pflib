#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( example ) {
  BOOST_TEST(true);
  BOOST_TEST(1==2);
  BOOST_TEST(1==1);
}
