/**
 * @file bindings.cxx
 * Python bindings of specific pflib functionality
 */

// the boost::bind library was updated and these updates
// had not made it into Boost.Python for the version we are using
#define BOOST_BIND_GLOBAL_PLACEHOLDERS

#include <boost/python.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/numpy.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/str.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python/tuple.hpp>
namespace bp = boost::python;

#include "pflib/Target.h"
#include "pflib/logging/Logging.h"
#include "pflib/version/Version.h"

class PyTarget {
  std::shared_ptr<pflib::Target> tgt_;

 public:
  PyTarget(bp::dict config) {
    // where we std::make_shared a dervied type of pflib::Target
    // depending on the setup
    std::cout << "creating { ";
    for (auto it = bp::stl_input_iterator<bp::tuple>(config.items());
         it != bp::stl_input_iterator<bp::tuple>(); it++) {
      bp::tuple kv = *it;
      auto key = bp::extract<const char*>(bp::str(kv[0]));
      auto val = bp::extract<const char*>(bp::str(kv[1]));
      std::cout << key << ": " << val << ", ";
    }
    std::cout << "}" << std::endl;

    tgt_.reset(
        pflib::makeTargetHcalBackplaneZCU(0 /*ilink*/, 0xf /*boardmask*/));
  }
  void configure() {
    // apply configuration stuff
    std::cout << "configure" << std::endl;
  }
  void start_run() {
    // prepare to collect data
    std::cout << "start_run" << std::endl;
    tgt_->setup_run(1 /*run*/, pflib::Target::DaqFormat::ECOND_SW_HEADERS,
                    42 /* contrib_id */);
  }
  std::vector<uint32_t> grab_pedestals() {
    tgt_->fc().sendL1A();
    usleep(100);
    return tgt_->read_event();
  }
  void end_run() {
    // stop data collection
    std::cout << "end_run" << std::endl;
  }
};

#include "pflib/packing/ECONDEventPacket.h"

/**
 * We could look at doing the Python->C++ conversion ourselves
 * but I kinda like the idea of binding std::vector<uint32_t> as
 * "WordVector" and then requiring the type to be solidified on
 * the Python side before calling decoding
// Template function to convert a python list to a C++ std::vector
template <typename T>
inline std::vector<T> py_list_to_std_vector(const boost::python::object&
iterable) { return
std::vector<T>(boost::python::stl_input_iterator<T>(iterable),
boost::python::stl_input_iterator<T>());
}

// https://slaclab.github.io/rogue/interfaces/stream/classes/frame.html
// the pyrogue.Frame can be converted toBytes or getNumpy and we probably
// want to pick which one so we don't do all this checking
// but we could do something like the below
void _from(pflib::packing::ECONDEventPacket& ep, boost::python::object frame) {
  auto class_name = frame.attr("__class__").attr("__name__");
  if (class_name == "list") {
    ep.from(py_list_to_std_vector<uint32_t>(frame));
  } else if (class_name == "bytearray") {
    throw std::runtime_error(class_name+" interpretation not implemented but
could be"); } else if (class_name == "numpy.ndarray") { std::vector<uint32_t>
cpp_frame(frame.shape(0)); uint32_t* frame_ptr =
reinterpret_cast<uint32_t*>(frame.get_data()); for (std::size_t i{0}; i <
frame.shape(0); i++) { cpp_frame[i] = *(frame_ptr + i);
    }
    ep.from(cpp_frame);
  } else {
    throw std::runtime_error("Unable to interpret type "+class_name);
  }
}
*/

/// do std::vector -> std::span conversion on C++ side
void _from(pflib::packing::ECONDEventPacket& ep, std::vector<uint32_t>& frame) {
  ep.from(frame);
}

/**
 * Initialize a submodule within the parent modulen named 'pypflib'.
 *
 * This creates a function called 'setup_<name>' that needs to be run
 * within the BOOST_PYTHON_MODULE(pypflib) block below.
 *
 * @param[in] name name of submodule
 */
#define BOOST_PYTHON_SUBMODULE(name)                                       \
  void setup_##name##_impl();                                              \
  void setup_##name() {                                                    \
    bp::object submodule(                                                  \
        bp::handle<>(bp::borrowed(PyImport_AddModule("pypflib." #name)))); \
    bp::scope().attr(#name) = submodule;                                   \
    bp::scope io_scope = submodule;                                        \
    setup_##name##_impl();                                                 \
  }                                                                        \
  void setup_##name##_impl()

BOOST_PYTHON_SUBMODULE(version) {
  bp::def("tag", pflib::version::tag);
  bp::def("git_describe", pflib::version::git_describe);
}

BOOST_PYTHON_SUBMODULE(logging) {
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

BOOST_PYTHON_MODULE(pypflib) {
  setup_version();
  setup_logging();

  bp::class_<PyTarget>("PyTarget", bp::init<bp::dict>())
      .def("configure", &PyTarget::configure)
      .def("start_run", &PyTarget::start_run)
      .def("grab_pedestals", &PyTarget::grab_pedestals)
      .def("end_run", &PyTarget::end_run);

  bp::class_<std::vector<uint32_t>>("WordVector")
      .def(bp::vector_indexing_suite<std::vector<uint32_t>>());

  bp::class_<pflib::packing::ECONDEventPacket>("ECONDEventPacket",
                                               bp::init<std::size_t>())
      .def("_from", &_from);
}
