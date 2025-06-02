#include "hgcroc_connection.h"

#include <boost/test/unit_test.hpp>

/// initial definition of fixture's static member
std::unique_ptr<pflib::Target> hgcroc_connection::tgt = nullptr;

hgcroc_connection::hgcroc_connection() {
  BOOST_TEST_MESSAGE("Connecting to HGCROC");
  tgt.reset(pflib::makeTargetFiberless());
}
