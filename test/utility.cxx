#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <cstdio>
#include <fstream>

#include "pflib/utility.h"

/**
 * a class that writes a temporary CSV when it is created,
 * holds the name, and deletes the CSV when destructed
 */
struct TempCSV {
  std::string file_path_;
  TempCSV(std::string_view content) {
    file_path_ = std::tmpnam(nullptr);
    std::ofstream f{file_path_};
    f << content << std::flush;
  }
  ~TempCSV() {
    std::remove(file_path_.c_str());
  }
};

BOOST_AUTO_TEST_SUITE(utility)

BOOST_AUTO_TEST_SUITE(load_csv)

BOOST_AUTO_TEST_CASE(well_behaved) {
  TempCSV t("#header,row,commented\n1,2,3\n4,5,6\n7,8,9");
  int val=0;
  pflib::loadIntegerCSV(t.file_path_,
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
  int val=0;
  pflib::loadIntegerCSV(t.file_path_,
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

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
