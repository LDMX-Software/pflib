/**
 * @file bindings.cxx
 * Python bindings of specific pflib functionality
 */

#include "bindings.h"

#include "logging.h"
#include "packing.h"
#include "pflib/Exception.h"
#include "pflib/TRIG.h"
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
namespace {

/// value of key in the Python dict, or the default if it is missing
template <typename T>
T get(const bp::dict& config, const char* key, T def) {
  if (not config.has_key(key)) return def;
  return bp::extract<T>(config[key]);
}

}  // namespace

class PyTarget {
  /// our handle to the pflib::Target
  std::shared_ptr<pflib::Target> tgt_;
  /// readout setup applied at configure and pre_start
  int econ_id_, samples_, soi_;
  bool external_, axis_;

 public:
  /// construct a PyTarget given a Python dict of parameters
  PyTarget(bp::dict config) {
    auto target = get<std::string>(config, "target", "hcal-zcu");
    auto dev = get<std::string>(config, "dev", "/dev/datadev_0");
    int ilink = get<int>(config, "ilink", 0);
    int mask =
        get<int>(config, "rocmask", target == "ecal-bittware" ? 0x3f : 0xf);
    econ_id_ = get<int>(config, "econ_id", ilink);
    samples_ = get<int>(config, "samples", 1);
    soi_ = get<int>(config, "soi", 0);
    external_ = get<bool>(config, "external", true);
    axis_ = get<bool>(config, "axis", true);

    if (target == "ecal-bittware") {
      tgt_.reset(pflib::makeTargetEcalSMMBittware(ilink, mask, dev.c_str()));
    } else if (target == "hcal-bittware") {
      tgt_.reset(
          pflib::makeTargetHcalBackplaneBittware(ilink, mask, dev.c_str()));
    } else if (target == "hcal-zcu") {
      tgt_.reset(pflib::makeTargetHcalBackplaneZCU(ilink, mask));
    } else {
      PFEXCEPTION_RAISE("BadTarget", "Unknown target '" + target + "'");
    }
  }
  /// readout setup of the capture slot and fast control, then take central
  /// L1As into it (pftool DAQ CONFIG and ENABLE); done here, not in pre_start,
  /// so the run-start ECR/EBR sent before the subsystems' Prestart get through
  void configure() {
    auto& daq = tgt_->daq();
    daq.setup(econ_id_, samples_, soi_);
    if (tgt_->trig()) tgt_->trig()->set_l1a_per_ror(samples_);
    tgt_->fc().setL1AperROR(samples_);
    daq.enable(true);
    tgt_->fc().fc_enables(true, external_);
    daq.AXIS_enable(axis_);
  }
  void trigger_align() {}
  void ror_latency() {}
  /// clear the capture slot (pftool DAQ RESET); setup_run is not used, its
  /// RUN_CLEAR upsets the front ends
  void pre_start() { tgt_->daq().reset(); }
  void go() {}
  void stop() {}
  void reset() {}
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

Parameters (all optional):
  target   "ecal-bittware", "hcal-bittware" or "hcal-zcu" (default)
  dev      PCIe device of a Bittware target (default "/dev/datadev_0")
  ilink    optical link, also the capture slot (default 0)
  rocmask  ROC (ECal) or board (HCal) mask (default 0x3f / 0xf)
  econ_id  ECON ID label of the capture slot (default ilink)
  samples  samples (L1As) per readout request (default 1)
  soi      sample of interest (default 0)
  external take L1As from the central readout requests (default True)
  axis     read out through AXIS/DMA (default True)
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
