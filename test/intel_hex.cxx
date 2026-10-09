#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "pflib/utility/intel_hex.h"
#include "helpers.h"

BOOST_AUTO_TEST_SUITE(intel_hex)

BOOST_AUTO_TEST_CASE(write_read_one_register) {
  TempFile image{"one_register.ihex", ""};
  {
    pflib::utility::intel_hex::Writer w{image.file_path_};
    pflib::utility::intel_hex::DataRecord d;
    d.from<uint32_t>(0x0102, 0x12345678);
    w.add(d);
    // writer closed on destruction
  } 

  {
    pflib::utility::intel_hex::Reader r{image.file_path_};
    bool checked_first{false};
    while (r.next()) {
      auto d = r.get();
      BOOST_CHECK(!checked_first);
      BOOST_CHECK_EQUAL(d.addr(), 0x0102);
      BOOST_CHECK_EQUAL(d.get<uint32_t>(), 0x12345678);
      checked_first = true;
    }
    BOOST_CHECK(checked_first);
  }
}

BOOST_AUTO_TEST_CASE(write_read_wide_register) {
  TempFile image{"wide_register.ihex", ""};
  {
    pflib::utility::intel_hex::Writer w{image.file_path_};
    pflib::utility::intel_hex::DataRecord d;
    d.from<uint64_t>(0xacdc, 0xabcdef0912345678ul);
    w.add(d);
    // writer closed on destruction
  } 

  {
    pflib::utility::intel_hex::Reader r{image.file_path_};
    bool checked_first{false};
    while (r.next()) {
      auto d = r.get();
      BOOST_CHECK(!checked_first);
      BOOST_CHECK_EQUAL(d.addr(), 0xacdc);
      BOOST_CHECK_EQUAL(d.get<uint64_t>(), 0xabcdef0912345678ul);
      checked_first = true;
    }
    BOOST_CHECK(checked_first);
  }
}

BOOST_AUTO_TEST_SUITE_END()
