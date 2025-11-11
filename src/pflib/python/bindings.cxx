#include <boost/python.hpp>

#include "pflib/Target.h"

class PyTarget {
  std::unique_ptr<pflib::Target> tgt_;
 public:
  PyTarget(/* determine which type of pflib::Target is made */);
  void configure(/* how to pass configuration */);
  void start_run();
  void end_run();
};

PyTarget::PyTarget() {
}

void PyTarget::configure() {
}

void PyTarget::start_run() {
}

void PyTarget::end_run() {
}

BOOST_PYTHON_MODULE(pypflib) {
  using namespace boost::python;
  class_<PyTarget>("PyTarget")
    .def("configure", &PyTarget::configure)
    .def("start_run", &PyTarget::start_run)
    .def("end_run", &PyTarget::end_run)
  ;
}
