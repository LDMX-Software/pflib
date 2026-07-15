#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <cstdio>

#include "helpers.h"
#include "pflib/utility/crc.h"
#include "pflib/utility/load_integer_csv.h"
#include "pflib/utility/str_to_int.h"

BOOST_AUTO_TEST_SUITE(utility)

BOOST_AUTO_TEST_SUITE(load_csv)

BOOST_AUTO_TEST_CASE(well_behaved) {
  TempFile t("pflib-test.csv", "#header,row,commented\n1,2,3\n4,5,6\n7,8,9");
  int val = 0;
  pflib::utility::load_integer_csv(t.file_path_,
                                   [&val](const std::vector<int>& row) {
                                     BOOST_CHECK(row.size() == 3);
                                     for (const int& cell : row) {
                                       val++;
                                       BOOST_CHECK(cell == val);
                                     }
                                   });
}

BOOST_AUTO_TEST_CASE(missing_cells) {
  TempFile t("pflib-test.csv", "#header,row,commented\n1,,3\n4,5,\n7,8,9");
  int val = 0;
  pflib::utility::load_integer_csv(t.file_path_,
                                   [&val](const std::vector<int>& row) {
                                     BOOST_CHECK(row.size() == 3);
                                     for (const int& cell : row) {
                                       val++;
                                       if (val == 2 or val == 6) {
                                         BOOST_CHECK(cell == 0);
                                       } else {
                                         BOOST_CHECK(cell == val);
                                       }
                                     }
                                   });
}

BOOST_AUTO_TEST_CASE(parse_header) {
  TempFile t("pflib-test.csv", "header, row,uncommented\n1,2,3\n4,5,6");
  int val = 0;
  pflib::utility::load_integer_csv(
      t.file_path_,
      [&val](const std::vector<int>& row) {
        BOOST_CHECK(row.size() == 3);
        for (const int& cell : row) {
          val++;
          BOOST_CHECK(cell == val);
        }
      },
      [](const std::vector<std::string>& row) {
        BOOST_CHECK(row.size() == 3);
        BOOST_CHECK(row[0] == "header");
        BOOST_CHECK(row[1] == " row");
        BOOST_CHECK(row[2] == "uncommented");
      });
}

BOOST_AUTO_TEST_CASE(multiline_header) {
  TempFile t("pflib-test.csv",
             "# some extra comment\nheader, row,uncommented\n1,2,3\n4,5,6");
  int val = 0;
  pflib::utility::load_integer_csv(
      t.file_path_,
      [&val](const std::vector<int>& row) {
        BOOST_CHECK(row.size() == 3);
        for (const int& cell : row) {
          val++;
          BOOST_CHECK(cell == val);
        }
      },
      [](const std::vector<std::string>& row) {
        BOOST_CHECK(row.size() == 3);
        BOOST_CHECK(row[0] == "header");
        BOOST_CHECK(row[1] == " row");
        BOOST_CHECK(row[2] == "uncommented");
      });
}

BOOST_AUTO_TEST_CASE(storage_params) {
  TempFile t("pflib-test.csv", "page.param1,page.param2\n1,20\n3,40\n");
  std::vector<std::string> param_names;
  std::vector<std::vector<int>> param_vals;
  pflib::utility::load_integer_csv(
      t.file_path_,
      [&param_vals](const std::vector<int>& row) { param_vals.push_back(row); },
      [&param_names](const std::vector<std::string>& row) {
        param_names = row;
      });
  BOOST_CHECK_EQUAL(param_names.size(), 2);
  BOOST_CHECK_EQUAL(param_names[0], "page.param1");
  BOOST_CHECK_EQUAL(param_names[1], "page.param2");
  BOOST_CHECK_EQUAL(param_vals.size(), 2);
  BOOST_CHECK_EQUAL(param_vals[0].size(), 2);
  BOOST_CHECK_EQUAL(param_vals[0][0], 1);
  BOOST_CHECK_EQUAL(param_vals[0][1], 20);
  BOOST_CHECK_EQUAL(param_vals[1].size(), 2);
  BOOST_CHECK_EQUAL(param_vals[1][0], 3);
  BOOST_CHECK_EQUAL(param_vals[1][1], 40);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(crc);

BOOST_AUTO_TEST_CASE(increment) {
  std::vector<uint32_t> data = {0x02};
  auto result = pflib::utility::crc32(data);
  BOOST_CHECK_EQUAL(result, 0x09823b6e);
}

BOOST_AUTO_TEST_CASE(econd_example_header_crc) {
  // Figure 34 from ECOND Spec
  uint64_t data{0x00aa5741000750ac};
  auto result = pflib::utility::econd_crc8(data);
  BOOST_CHECK_EQUAL(result, 0xfc);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(str_to_int) {
  using pflib::utility::is_integer;
  BOOST_CHECK(not is_integer("foo"));
  BOOST_CHECK(is_integer(" 0xaf"));
  BOOST_CHECK(is_integer("0b011"));
  BOOST_CHECK(not is_integer("0b0123"));
  BOOST_CHECK(not is_integer("12cd"));
  BOOST_CHECK(is_integer("0X9Ce"));

  using pflib::utility::str_to_int;
  BOOST_CHECK_EQUAL(str_to_int("0xaf"), 0xaf);
  BOOST_CHECK_EQUAL(str_to_int("0b011"), 0b011);
  BOOST_CHECK_EQUAL(str_to_int("0X9Ce"), 0x9ce);
  BOOST_CHECK_EQUAL(str_to_int("1234"), 1234);
  BOOST_CHECK_EQUAL(str_to_int("0x12345678"), 0x12345678);

  // negative numbers get their negative bit cast to being apart
  // of the magnitude and thus overflow
  BOOST_CHECK_THROW(str_to_int("-12"), std::range_error);

  // use str_to_ullint if you need to support more than
  // 32 bits, 8 hex digits, etc...
  BOOST_CHECK_THROW(str_to_int("0x123456787654321"), std::range_error);
  using pflib::utility::str_to_ullint;
  BOOST_CHECK_EQUAL(str_to_ullint("0x123456787654321"), 0x123456787654321ull);
}

BOOST_AUTO_TEST_SUITE_END()
