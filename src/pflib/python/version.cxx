#include "version.h"
#include "pflib/version/Version.h"
void setup_version() {
  BOOST_PYTHON_SUBMODULE(version);
  bp::def("tag", pflib::version::tag);
  bp::def("git_describe", pflib::version::git_describe);
}

