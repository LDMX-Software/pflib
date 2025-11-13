#include "logging.h"
#include "pflib/logging/Logging.h"

void setup_logging() {
  BOOST_PYTHON_SUBMODULE(logging);
  bp::enum_<pflib::logging::level>("level")
      .value("trace", pflib::logging::level::trace)
      .value("debug", pflib::logging::level::debug)
      .value("info", pflib::logging::level::info)
      .value("warn", pflib::logging::level::warn)
      .value("error", pflib::logging::level::error)
      .value("fatal", pflib::logging::level::fatal);
  bp::def("open", pflib::logging::open, (bp::arg("color")));
  bp::def("set", pflib::logging::set, (bp::arg("lvl"), bp::arg("only") = ""));
  bp::def("close", pflib::logging::close);
}
