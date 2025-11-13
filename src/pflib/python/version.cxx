#include "version.h"
#include "pflib/version/Version.h"
void setup_version() {
  /**
   * For future debugging, see what specific version of pflib
   * is being used from the Python bindings.
   */
  BOOST_PYTHON_SUBMODULE(version);
  bp::def("tag", pflib::version::tag,
      "The tag that pflib was built with");
  bp::def("git_describe", pflib::version::git_describe,
      "The full output of `git describe` for the build of pflib.");
}

