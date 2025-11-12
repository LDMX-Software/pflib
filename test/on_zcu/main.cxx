#define BOOST_TEST_MODULE test pflib
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "hgcroc_connection.h"
#include "pflib/logging/Logging.h"

using pflib_logging_fixture = pflib::logging::fixture;

BOOST_GLOBAL_FIXTURE(pflib_logging_fixture);
BOOST_GLOBAL_FIXTURE(hgcroc_connection);
