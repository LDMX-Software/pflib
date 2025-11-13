/**
 * @file bindings.cxx
 * Python bindings of specific pflib functionality
 */

#include "bindings.h"
#include "logging.h"
#include "packing.h"
#include "version.h"
#include "pflib/Target.h"

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

BOOST_PYTHON_MODULE(pypflib) {
  setup_version();
  setup_logging();
  setup_packing();

  bp::class_<PyTarget>("PyTarget", bp::init<bp::dict>())
      .def("configure", &PyTarget::configure)
      .def("start_run", &PyTarget::start_run)
      .def("grab_pedestals", &PyTarget::grab_pedestals)
      .def("end_run", &PyTarget::end_run);

}
