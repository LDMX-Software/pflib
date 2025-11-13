/**
 * @file bindings.cxx
 * Python bindings of specific pflib functionality
 */

#include "bindings.h"

#include "logging.h"
#include "packing.h"
#include "pflib/Target.h"
#include "version.h"

/**
 * Hold a pflib::Target to do run commands on
 *
 * We need to hold a pflib::Target and can't bind pflib::Target's
 * directly since pflib::Target is abstract. We could /maybe/ use
 * bp::no_copy in order to allow binding of an abstract class, but
 * I think this is cleaner anyways since we need a spot to map the
 * Python Run Control commands onto C++ pflib::Target functions (or
 * series of functions).
 *
 * See this example:
 * https://github.com/slaclab/ucsc-hn/blob/master/firmware/python/ucsc_hn/_RenaDataEmulator.py
 * The `self._processor` object is the bound C++ and you can see how
 * we can connect `pyrogue.LocalVariable` to specific setters/getters
 * here in order to ease passage of configuration between rogue and us.
 */
class PyTarget {
  /// our handle to the pflib::Target
  std::shared_ptr<pflib::Target> tgt_;

 public:
  /// construct a PyTarget given some arbitrary Python dict of parameters
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
  void trigger_align() { std::cout << "trigger_align" << std::endl; }
  void ror_latency() { std::cout << "ror_latency" << std::endl; }
  void pre_start() {
    // prepare to collect data
    std::cout << "start_run" << std::endl;
    tgt_->setup_run(1 /*run*/, pflib::Target::DaqFormat::ECOND_SW_HEADERS,
                    42 /* contrib_id */);
  }
  void go() { std::cout << "go" << std::endl; }
  void stop() { std::cout << "stop" << std::endl; }
  void reset() { std::cout << "reset" << std::endl; }
  /// should not use in actual DAQ
  std::vector<uint32_t> grab_pedestals() {
    tgt_->fc().sendL1A();
    usleep(100);
    return tgt_->read_event();
  }
};

static const char* PyTarget__doc__ = R"DOC(Hold a pflib::Target

This class holds a C++ pflib::Target that we can then do
run commands on. The main configuration parameters are passed
into the object via a Python dict.
)DOC";

static const char* PyTarget__init____doc__ = R"DOC(construct a pflib::Target

Given a dictionary of configuration parameters, construct a C++ pflib::Target
and await further run commands.
)DOC";

static const char* PyTarget_grab_pedestals__doc__ =
    R"DOC(DO NOT USE IN RUN CONTROL

This function was bound to make sure the python bindings were functional while
waiting for the Bittware firmware to be written.
Hopefully, we will remember to remove it as the software progresses, but
if its still around it should not be present in any Rogue Run Control code.
)DOC";

BOOST_PYTHON_MODULE(pypflib) {
  setup_version();
  setup_logging();
  setup_packing();

  bp::class_<PyTarget>("PyTarget", PyTarget__doc__,
                       bp::init<bp::dict>(PyTarget__init____doc__,
                                          (bp::arg("config") = bp::dict())))
      .def("configure", &PyTarget::configure)
      .def("trigger_align", &PyTarget::trigger_align)
      .def("ror_latency", &PyTarget::ror_latency)
      .def("pre_start", &PyTarget::pre_start)
      .def("go", &PyTarget::go)
      .def("stop", &PyTarget::stop)
      .def("reset", &PyTarget::reset)
      .def("grab_pedestals", &PyTarget::grab_pedestals,
           PyTarget_grab_pedestals__doc__);
}
