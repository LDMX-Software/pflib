#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "pflib/utility/intel_hex.h"
#include "helpers.h"

/// load the entire file's contents into a string
std::string load(const std::string& filepath) {
  std::ostringstream buf;
  std::ifstream f{filepath};
  buf << f.rdbuf();
  return buf.str();
}

/// find and replace in a string
void find_and_replace_first(
    std::string& str,
    const std::string& pattern,
    const std::string& replacement
) {
  std::size_t first_pos = str.find(pattern);
  if (first_pos == std::string::npos) return;
  str.replace(first_pos, pattern.length(), replacement);
}

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
    BOOST_CHECK_EQUAL(r.errors(), 0);
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
    BOOST_CHECK_EQUAL(r.errors(), 0);
  }
}

BOOST_AUTO_TEST_CASE(check_corruption) {
  TempFile image{"corrupted.ihex", ""};
  {
    pflib::utility::intel_hex::Writer w{image.file_path_};
    pflib::utility::intel_hex::DataRecord d;
    d.from<uint64_t>(0xacdc, 0xabcdef0912345678ul);
    w.add(d);
    // writer closed on destruction
  }

  {
    BOOST_TEST_MESSAGE("checking that reader warns on a corrupted value");
    {
      std::string content{load(image.file_path_)};
      find_and_replace_first(content, "09", "90");
      std::ofstream f(image.file_path_);
      f << content << std::flush;
    }
     
    pflib::utility::intel_hex::Reader r{image.file_path_};
    bool checked_first{false};
    while (r.next()) {
      auto d = r.get();
      BOOST_CHECK(!checked_first);
      BOOST_CHECK_EQUAL(d.addr(), 0xacdc);
      // we can still read the (corrupted) value
      BOOST_CHECK_EQUAL(d.get<uint64_t>(), 0xabcdef9012345678ul);
      checked_first = true;
    }
    BOOST_CHECK(checked_first);
    // but there are now errors
    BOOST_CHECK_EQUAL(r.errors(), 1);
  }
}

BOOST_AUTO_TEST_SUITE_END()
