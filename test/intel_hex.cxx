#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <random>

#include "pflib/utility/intel_hex.h"
#include "pflib/Compile.h"
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

BOOST_AUTO_TEST_SUITE(check_corruption)

BOOST_AUTO_TEST_CASE(corrupted_value) {
  TempFile image{"corrupted_value.ihex",
    R"IHEX(:08acdc01abcdef9012345678eb
:00000001FF
)IHEX"};
  BOOST_TEST_MESSAGE("checking that reader warns on a corrupted value");
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

BOOST_AUTO_TEST_CASE(bad_line) {
  TempFile image{"bad_line.ihex",
    R"IHEX(foo
:08acdc01abcdef0912345678eb
:00000001FF
)IHEX"};
  BOOST_TEST_MESSAGE("checking that reader warns on a bad line");
  pflib::utility::intel_hex::Reader r{image.file_path_};
  bool checked_first{false};
  while (r.next()) {
    auto d = r.get();
    BOOST_CHECK(!checked_first);
    BOOST_CHECK_EQUAL(d.addr(), 0xacdc);
    // we can still read the other stuff
    BOOST_CHECK_EQUAL(d.get<uint64_t>(), 0xabcdef0912345678ul);
    checked_first = true;
  }
  BOOST_CHECK(checked_first);
  // but there are now errors
  BOOST_CHECK_EQUAL(r.errors(), 1);
}

BOOST_AUTO_TEST_CASE(no_eof_line) {
  TempFile image{"no_eof_line.ihex",
    R"IHEX(:08acdc01abcdef0912345678eb
)IHEX"};
  BOOST_TEST_MESSAGE("checking that reader warns on missing EOF record");
  pflib::utility::intel_hex::Reader r{image.file_path_};
  bool checked_first{false};
  while (r.next()) {
    auto d = r.get();
    BOOST_CHECK(!checked_first);
    BOOST_CHECK_EQUAL(d.addr(), 0xacdc);
    // we can still read the other stuff
    BOOST_CHECK_EQUAL(d.get<uint64_t>(), 0xabcdef0912345678ul);
    checked_first = true;
  }
  BOOST_CHECK(checked_first);
  // but there are now errors
  BOOST_CHECK_EQUAL(r.errors(), 1);
}

BOOST_AUTO_TEST_CASE(ignore_comments) {
  TempFile image{"ignore_comments.ihex",
    R"IHEX(// we ignore lines starting with two slashes
:08acdc01abcdef0912345678eb

:00000001FF
)IHEX"};
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

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(bulk)

// random data to fuzz test the checksum
BOOST_AUTO_TEST_CASE(one_thousand) {
  // same seed and distribution for writing and reading
  static const int seed = 1;
  std::uniform_int_distribution<uint32_t> rand_value{};
  TempFile image{"random.ihex", ""};
  {
    // writing block
    std::mt19937 gen(seed);
    pflib::utility::intel_hex::Writer w{image.file_path_};
    pflib::utility::intel_hex::DataRecord d;
    for (uint16_t addr{0}; addr < 1000; addr++) {
      d.from<uint32_t>(addr, rand_value(gen));
      w.add(d);
    }
  }

  {
    // re-seed so we have the same sequence when reading
    std::mt19937 gen(seed);
    uint16_t addr{0};
    pflib::utility::intel_hex::Reader r{image.file_path_};
    while (r.next()) {
      auto d = r.get();
      BOOST_CHECK_EQUAL(d.addr(), addr);
      BOOST_CHECK_EQUAL(d.get<uint32_t>(), rand_value(gen));
      addr++;
    }
    BOOST_CHECK_EQUAL(r.errors(), 0);
  }
}

// sparse data from with right scale of image
BOOST_AUTO_TEST_CASE(roc_defaults) {
  pflib::Compiler c = pflib::Compiler::get("sipm_rocv3");
  auto default_registers = c.compile(c.defaults());
  TempFile image{"roc_defaults.ihex", ""};
  {
    pflib::utility::intel_hex::Writer w{image.file_path_};
    pflib::utility::intel_hex::DataRecord d;
    for (const auto& [page_addr, page] : default_registers) {
      for (const auto& [reg_addr, value] : page) {
        d.from_bytes(((page_addr << 5) | reg_addr), {value});
        w.add(d);
      }
    }
  }

  {
    pflib::utility::intel_hex::Reader r{image.file_path_};
    while (r.next()) {
      auto d = r.get();
      int page = (d.addr() >> 5);
      int reg = (d.addr() & 0x1f);
      BOOST_CHECK_EQUAL(d.get_bytes().at(0), default_registers[page][reg]);
    }
    BOOST_CHECK_EQUAL(r.errors(), 0);
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
