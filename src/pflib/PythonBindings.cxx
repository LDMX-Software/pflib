/**
 * rogue delegates the definition of python bindings to static
 * class methods across all of their files. This makes sense for 
 * such a large code base; however, for a smaller codebase like pflib
 * isolating the python bindings to one file takes priority.
 */

#include <boost/python.hpp>
namespace bp = boost::python;

#include "pflib/WishboneInterface.h"
#include "pflib/I2C.h"
#include "pflib/GPIO.h"
#include "pflib/ROC.h"

namespace pflib {
namespace python {

/**
 * Need to define pointers for overloading function names
 * Not working rn...
bool (*GPIO_getGPI_single)(int)        = &GPIO::getGPI;
std::vector<bool> (*GPIO_getGPI_all)() = &GPIO::getGPI;
void (*GPIO_setGPO_single)(int,bool)   = &GPIO::setGPO;
void (*GPIO_setGPO_all)(const std::vector<bool>&) = &GPIO::setGPO;
 */

}  // namespace python
}  // namespace pflib

BOOST_PYTHON_MODULE(pflib) {

  /// abstract base
  bp::class_<pflib::WishboneInterface>("WishboneInterface", bp::no_init);

  /**
   * Could combine set/get into a r/w python property
   *  https://www.boost.org/doc/libs/1_75_0/libs/python/doc/html/tutorial/tutorial/exposing.html#tutorial.exposing.class_properties
   */
  bp::class_<pflib::I2C>("I2C",bp::init<pflib::WishboneInterface,int>())
    .def("set_active_bus", &pflib::I2C::set_active_bus)
    .def("get_active_bus", &pflib::I2C::get_active_bus)
    .def("get_bus_count", &pflib::I2C::get_bus_count)
    .def("set_bus_speed", &pflib::I2C::set_bus_speed)
    .def("get_bus_speed", &pflib::I2C::get_bus_speed)
    .def("write_byte", &pflib::I2C::write_byte)
    .def("read_byte", &pflib::I2C::read_byte)
    .def("general_write_read", &pflib::I2C::general_write_read)
    .def("backplane_hack", &pflib::I2C::backplane_hack)
  ;

  /*
  bp::class_<pflib::GPIO>("GPIO",bp::init<pflib::WishboneInterface,int>())
    .def("getGPOcount", &pflib::GPIO::getGPOcount)
    .def("getGPIcount", &pflib::GPIO::getGPIcount)
    .def("getGPI", pflib::python::GPIO_getGPI_single)
    .def("getGPI", pflib::python::GPIO_getGPI_all)
    .def("setGPO", pflib::python::GPIO_setGPO_single)
    .def("setGPO", pflib::python::GPIO_setGPO_all)
    .def("getGPO", &pflib::GPIO::getGPO)
  ;
  */

  bp::class_<pflib::ROC>("ROC",bp::init<pflib::I2C,int>())
    .def("readPage", &pflib::ROC::readPage)
    .def("setValue", &pflib::ROC::setValue)
    .def("getChannelParameters", &pflib::ROC::getChannelParameters)
    .def("setChannelParameters", &pflib::ROC::setChannelParameters)
  ;
};
