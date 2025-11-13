#include "logging.h"

#include "pflib/logging/Logging.h"

void setup_logging() {
  /**
   * We bind the logging controls because the decoding and the slow
   * control emit messages to the logging so we should be able to control
   * its level.
   */
  BOOST_PYTHON_SUBMODULE(logging);

  /// the level enum gets mapped into Python
  bp::enum_<pflib::logging::level>(
      "level", "See pflib::logging::level for documentation")
      .value("trace", pflib::logging::level::trace)
      .value("debug", pflib::logging::level::debug)
      .value("info", pflib::logging::level::info)
      .value("warn", pflib::logging::level::warn)
      .value("error", pflib::logging::level::error)
      .value("fatal", pflib::logging::level::fatal);

  bp::def("open", pflib::logging::open, (bp::arg("color")),
          "See pflib::logging::open");
  bp::def("set", pflib::logging::set, (bp::arg("lvl"), bp::arg("only") = ""),
          "See pflib::logging::set");
  bp::def("close", pflib::logging::close, "See pflib::logging::close");
}
