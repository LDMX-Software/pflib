#define BOOST_TEST_DYN_LINK
#include "pflib/Parameters.h"

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include <iomanip>

#include "helpers.h"
#include "pflib/Exception.h"

BOOST_AUTO_TEST_SUITE(parameters)

BOOST_AUTO_TEST_CASE(one_depth) {
  TempFile t("pflib-parameters-test.yaml",
             R"YAML(# this is a shallow yaml
one: 1
two: 2.0
three: "three"
four: [1, 2, 3, 4]
five:
- 1.0
- 2.0
- 3.0
sub:
  one: 1.0
  two: ["one", "two"]
)YAML");
  auto p{pflib::Parameters::from_yaml(t.file_path_)};
  BOOST_CHECK_EQUAL(p.get<int>("one"), 1);
  BOOST_CHECK_EQUAL(p.get<double>("two"), 2.0);
  BOOST_CHECK_EQUAL(p.get<std::string>("three"), "three");
  std::vector<int> four = {1, 2, 3, 4};
  auto read_four{p.get<std::vector<int>>("four")};
  BOOST_CHECK_EQUAL_COLLECTIONS(read_four.begin(), read_four.end(),
                                four.begin(), four.end());
  std::vector<double> five = {1.0, 2.0, 3.0};
  auto read_five{p.get<std::vector<double>>("five")};
  BOOST_CHECK_EQUAL_COLLECTIONS(read_five.begin(), read_five.end(),
                                five.begin(), five.end());
  auto read_sub{p.get<pflib::Parameters>("sub")};
  BOOST_CHECK_EQUAL(read_sub.get<double>("one"), 1.0);
  auto read_sub_two{read_sub.get<std::vector<std::string>>("two")};
  std::vector<std::string> sub_two = {"one", "two"};
  BOOST_CHECK_EQUAL_COLLECTIONS(read_sub_two.begin(), read_sub_two.end(),
                                sub_two.begin(), sub_two.end());
}

BOOST_AUTO_TEST_SUITE_END()
