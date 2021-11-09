/**
 * rogue delegates the definition of python bindings to static
 * class methods across all of their files. This makes sense for 
 * such a large code base; however, for a smaller codebase like pflib
 * isolating the python bindings to one file takes priority.
 */

#include <boost/python.hpp>

#include "pflib/Bias.h"
#include "pflib/WishboneInterface.h"

namespace pflib {
namespace python {

struct WishboneInterfaceWrapper : WishboneInterface, boost::python::wrapper<WishboneInterface> {
  void wb_write(int target, uint32_t addr, uint32_t data) {
    this->get_override("wb_write")(target, addr, data);
  }
};

}
}

BOOST_PYTHON_MODULE(pflib) {

  boost::python::class_<pflib::python::WishboneInterfaceWrapper, boost::noncopyable>("WishboneInterface")
    .def("wb_write", boost::python::pure_virtual(&pflib::WishboneInterface::wb_write))
  ;
 
  boost::python::class_<pflib::MAX5825>("MAX5825")
    .def("set", &pflib::MAX5825::set)
    .def("get", &pflib::MAX5825::get)
    .def("setByDAC", &pflib::MAX5825::setByDAC)
    .def("getByDAC", &pflib::MAX5825::getByDAC)
    .def("setRefVoltage", &pflib::MAX5825::setRefVoltage)
  ;
};
