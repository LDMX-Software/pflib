#include "packing.h"
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


/// convert std::vector to std::span on C++ side
void from_word_vector(pflib::packing::ECONDEventPacket& ep,
                      std::vector<uint32_t>& wv) {
  ep.from(wv);
}

void setup_packing() {
  BOOST_PYTHON_SUBMODULE(packing);
  bp::class_<std::vector<uint32_t>>("WordVector")
      .def(bp::vector_indexing_suite<std::vector<uint32_t>>());

  bp::class_<pflib::packing::Sample>("Sample", bp::init<>())
      .add_property("Tc", &pflib::packing::Sample::Tc)
      .add_property("Tp", &pflib::packing::Sample::Tp)
      .add_property("toa", &pflib::packing::Sample::toa)
      .add_property("adc_tm1", &pflib::packing::Sample::adc_tm1)
      .add_property("adc", &pflib::packing::Sample::adc)
      .add_property("tot", &pflib::packing::Sample::tot)
      .def_readwrite("word", &pflib::packing::Sample::word);

  bp::class_<pflib::packing::ECONDEventPacket>("ECONDEventPacket",
                                               bp::init<std::size_t>())
      .def("from_word_vector", &from_word_vector)
      .def("adc_cm0", &pflib::packing::ECONDEventPacket::adc_cm0)
      .def("adc_cm1", &pflib::packing::ECONDEventPacket::adc_cm1)
      .def("channel", &pflib::packing::ECONDEventPacket::channel)
      .def("calib", &pflib::packing::ECONDEventPacket::calib);
}
