#include "load_parameter_points.h"

#include "pflib/Exception.h"
#include "pflib/utility/load_integer_csv.h"

std::tuple<std::vector<std::pair<std::string,std::string>>, std::vector<std::vector<int>>>
load_parameter_points(const std::string& filepath) {
  /**
   * The input parameter points file is just a CSV where the header
   * is used to define the parameters that will be set and the rows
   * are the values of those parameters.
   *
   * For example, the CSV
   * ```csv
   * page.param1,page.param2
   * 1,2
   * 3,4
   * ```
   * would produce two runs with this command where the parameter settings are
   * 1. page.param1 = 1 and page.param2 = 2
   * 2. page.param1 = 3 and page.param2 = 4
   */
  std::vector<std::pair<std::string,std::string>> param_names;
  std::vector<std::vector<int>> param_values;
  pflib::utility::load_integer_csv(
    filepath,
    [&param_names,&param_values,&filepath]
    (const std::vector<int>& row) {
      if (row.size() != param_names.size()) {
        PFEXCEPTION_RAISE("BadRow",
            "A row in "+std::string(filepath)
            +" contains "+std::to_string(row.size())
            +" cells which is not "+std::to_string(param_names.size())
            +" the number of parameters defined in the header.");
      }
      param_values.push_back(row);
    },
    [&param_names](const std::vector<std::string>& header) {
      param_names.resize(header.size());
      for (std::size_t i{0}; i < header.size(); i++) {
        const auto& param_fullname = header[i];
        auto dot = param_fullname.find(".");
        if (dot == std::string::npos) {
          PFEXCEPTION_RAISE("BadParam",
              "Header cell "+param_fullname+" does not contain a '.' "
              "separating the page and parameter names.");
        }
        param_names[i] = {
          param_fullname.substr(0, dot),
          param_fullname.substr(dot+1)
        };
      }
    }
  ); 
  return std::make_tuple(param_names, param_values);
}
