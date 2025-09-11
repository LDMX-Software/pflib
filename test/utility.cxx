#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <cstdio>
#include <filesystem>
#include <fstream>

#include "pflib/utility/crc.h"
#include "pflib/utility/load_integer_csv.h"

/**
 * a class that writes a temporary CSV when it is created,
 * holds the name, and deletes the CSV when destructed
 */
struct TempCSV {
  std::string file_path_;
  TempCSV(std::string_view content) {
    file_path_ =
        (std::filesystem::temp_directory_path() / "pflib-test-utility.csv");
    std::ofstream f{file_path_};
    f << content << std::flush;
  }
  ~TempCSV() { std::remove(file_path_.c_str()); }
};

BOOST_AUTO_TEST_SUITE(utility)

BOOST_AUTO_TEST_SUITE(load_csv)

BOOST_AUTO_TEST_CASE(well_behaved) {
  TempCSV t("#header,row,commented\n1,2,3\n4,5,6\n7,8,9");
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
  TempCSV t("#header,row,commented\n1,,3\n4,5,\n7,8,9");
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
  TempCSV t("header, row,uncommented\n1,2,3\n4,5,6");
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
  TempCSV t("# some extra comment\nheader, row,uncommented\n1,2,3\n4,5,6");
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
  TempCSV t("page.param1,page.param2\n1,20\n3,40\n");
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

BOOST_AUTO_TEST_SUITE_END()
