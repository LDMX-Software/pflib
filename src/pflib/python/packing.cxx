#include "packing.h"

#include <iostream>

#include "pflib/packing/ECONDEventPacket.h"
#include "pflib/packing/MultiSampleECONDEventPacket.h"

/*
// Template function to convert a python list to a C++ std::vector
template <typename T>
inline std::vector<T> py_list_to_std_vector(
    const boost::python::object& iterable) {
  return std::vector<T>(boost::python::stl_input_iterator<T>(iterable),
      boost::python::stl_input_iterator<T>());
}

std::vector<uint32_t> np_to_word_vector(boost::python::numpy::ndarray arr) {
  // check dtype and "view" as uint32 if not already
  std::vector<uint32_t> cpp_frame(arr.shape(0));
  uint32_t* arr_ptr = reinterpret_cast<uint32_t*>(arr.get_data());
  for (std::size_t i{0}; i < arr.shape(0); i++) {
    cpp_frame[i] = *(arr_ptr + i);
  }
  return cpp_frame;
}

// https://slaclab.github.io/rogue/interfaces/stream/classes/frame.html
// the pyrogue.Frame can be converted toBytes or getNumpy and we probably
// want to pick which one so we don't do all this checking
std::vector<uint32_t> to_word_vector(boost::python::object frame) {
  auto class_name = frame.attr("__class__").attr("__name__");
  if (class_name == "list") {
    return py_list_to_std_vector<uint32_t>(frame);
  } else if (class_name == "bytearray") {
    throw std::runtime_error("bytearray interpretation not implemented but could
be"); } else if (class_name == "numpy.ndarray") { return
np_to_word_vector(frame); } else { throw std::runtime_error("Unable to interpret
type "+bp::str(class_name));
  }
}
*/

/// wrapper to call EventPacket::from given a WordVector
template <class EventPacket>
void _from(EventPacket& ep, std::vector<uint32_t>& wv) {
  // auto wv = to_word_vector(frame);
  ep.from(wv);
}

const char* WordVector__doc__ =
    R"DOC(raw data packet acceptable by decoding classes

This is a std::vector<uint32_t> which is the representation of raw data in pflib.
The bindings provide normal `list`-like access with functions like `extend`, `append`,
and slicing (e.g. `v[1:]`).
)DOC";

const char* ECONDEventPacket__doc__ =
    R"DOC(representation for a packet returned by an ECON-D

This is helpful for decoding a ECON-D event packet.
The translation from Python data structures into C++ data structures
could be slow due to the amount of copying that may be present,
so this decoding should be done outside of the main DAQ stream.
)DOC";

const char* MultiSampleECONDEventPacket__doc__ =
    R"DOC(one event potentially containing multiple samples

This packets holds multiple ECONDEventPackets and is helpful
for decoding an output event which may contain multiple samples.
Even for events that are known to have only one sample, the
firmware (and the software's emulation of it) insert extra headers/trailers
that this decoder can handle.
)DOC";

std::size_t MultiSampleECONDEventPacket_n_samples(
    const pflib::packing::MultiSampleECONDEventPacket& ep) {
  return ep.samples.size();
}

const pflib::packing::ECONDEventPacket& MultiSampleECONDEventPacket_sample(
    const pflib::packing::MultiSampleECONDEventPacket& ep, std::size_t index) {
  return ep.samples.at(index);
}

struct OFStream {
  std::shared_ptr<std::ofstream> output_;
  void open(const std::string& fp) {
    if (!output_) {
      output_ = std::make_shared<std::ofstream>();
    }
    output_->open(fp);
  }
  bool is_open() {
    if (!output_) return false;
    return output_->is_open();
  }
  void close() {
    if (!output_) return;
    output_->close();
  }
};

void MultiSampleECONDEventPacket_header_to_csv(
    pflib::packing::MultiSampleECONDEventPacket& ep, OFStream o) {
  if (not o.is_open()) {
    throw std::runtime_error("OFStream has not been opened.");
  }
  (*o.output_) << pflib::packing::MultiSampleECONDEventPacket::to_csv_header
               << '\n';
}

void MultiSampleECONDEventPacket_to_csv(
    pflib::packing::MultiSampleECONDEventPacket& ep, OFStream o) {
  if (not o.is_open()) {
    throw std::runtime_error("OFStream has not been opened.");
  }
  ep.to_csv(*(o.output_));
}

void setup_packing() {
  BOOST_PYTHON_SUBMODULE(packing);

  /*
  bp::def("to_word_vector", "convert input python object into a WordVector",
      to_word_vector);
      */

  bp::class_<OFStream, boost::noncopyable>("OFStream", bp::init<>())
      .def("open", &OFStream::open)
      .def("is_open", &OFStream::is_open)
      .def("close", &OFStream::close);

  /**
   * We bind our standard raw-data object as a "WordVector"
   * so that we can construct one in Python to pass to the
   * decoding infrastructure.
   *
   * The bp::vector_indexing_suite provides the normal list-like
   * functions including `extend`, `append`, and slicing (e.g. `v[1:]`)
   * that make it possible to operate on `WordVector` like a `list`.
   */
  bp::class_<std::vector<uint32_t>>("WordVector", WordVector__doc__)
      .def(bp::vector_indexing_suite<std::vector<uint32_t>>());

  /**
   * Bind the Sample to `Sample` so that the interpretation of
   * a 32-bit sample word into measurements from a HGCROC is
   * available
   */
  bp::class_<pflib::packing::Sample>("Sample", bp::init<>())
      .add_property("Tc", &pflib::packing::Sample::Tc)
      .add_property("Tp", &pflib::packing::Sample::Tp)
      .add_property("toa", &pflib::packing::Sample::toa)
      .add_property("adc_tm1", &pflib::packing::Sample::adc_tm1)
      .add_property("adc", &pflib::packing::Sample::adc)
      .add_property("tot", &pflib::packing::Sample::tot)
      .def_readwrite("word", &pflib::packing::Sample::word);

  /**
   * Skip over the DAQLinkFrame and use accessors in the ECONDEventPacket
   * because it felt icky to have that many intermediate layers bound to
   * Python.
   */
  bp::class_<pflib::packing::ECONDEventPacket>(
      "ECONDEventPacket", ECONDEventPacket__doc__, bp::init<std::size_t>())
      .def("from_word_vector", &_from<pflib::packing::ECONDEventPacket>)
      .def("adc_cm0", &pflib::packing::ECONDEventPacket::adc_cm0)
      .def("adc_cm1", &pflib::packing::ECONDEventPacket::adc_cm1)
      .def("channel", &pflib::packing::ECONDEventPacket::channel)
      .def("calib", &pflib::packing::ECONDEventPacket::calib);

  bp::class_<pflib::packing::MultiSampleECONDEventPacket>(
      "MultiSampleECONDEventPacket", MultiSampleECONDEventPacket__doc__,
      bp::init<int>())
      .def("from_word_vector",
           &_from<pflib::packing::MultiSampleECONDEventPacket>)
      .def("soi", &pflib::packing::MultiSampleECONDEventPacket::soi,
           bp::return_value_policy<bp::reference_existing_object>())
      .add_property("n_samples", &MultiSampleECONDEventPacket_n_samples)
      .def("sample", &MultiSampleECONDEventPacket_sample,
           bp::return_value_policy<bp::reference_existing_object>())
      .def_readonly("i_soi",
                    &pflib::packing::MultiSampleECONDEventPacket::i_soi)
      .def_readonly("econd_id",
                    &pflib::packing::MultiSampleECONDEventPacket::econd_id)
      .def("to_csv", &MultiSampleECONDEventPacket_to_csv)
      .def("header_to_csv", &MultiSampleECONDEventPacket_header_to_csv);
}
