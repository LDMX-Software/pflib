/**
 * @file bindings.cxx
 * Python bindings of specific pflib functionality
 */

// the boost::bind library was updated and these updates
// had not made it into Boost.Python for the version we are using
#define BOOST_BIND_GLOBAL_PLACEHOLDERS

#include <boost/python/dict.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/str.hpp>
#include <boost/python.hpp>
using namespace boost::python;

#include "pflib/Target.h"

class PyTarget {
  std::shared_ptr<pflib::Target> tgt_;
 public:
  PyTarget(dict config) {
    // where we std::make_shared a dervied type of pflib::Target
    // depending on the setup
    std::cout << "creating { ";
    for (auto it = stl_input_iterator<tuple>(config.items()); it != stl_input_iterator<tuple>(); it++) {
      tuple kv = *it;
      auto key = extract<const char*>(str(kv[0]));
      auto val = extract<const char*>(str(kv[1]));
      std::cout << key << ": " << val << ", ";
    }
    std::cout << "}" << std::endl;
  }
  void configure() {
    // apply configuration stuff
    std::cout << "configure" << std::endl;
  }
  void start_run() {
    // prepare to collect data
    std::cout << "start_run" << std::endl;
  }
  void end_run() {
    // stop data collection
    std::cout << "end_run" << std::endl;
  }
};

BOOST_PYTHON_MODULE(pypflib) {
  using namespace boost::python;
  class_<PyTarget>("PyTarget", init<boost::python::dict>())
    .def("configure", &PyTarget::configure)
    .def("start_run", &PyTarget::start_run)
    .def("end_run", &PyTarget::end_run)
  ;
}
