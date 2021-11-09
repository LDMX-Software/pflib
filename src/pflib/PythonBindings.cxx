/**
 * rogue delegates the definition of python bindings to static
 * class methods across all of their files. This makes sense for 
 * such a large code base; however, for a smaller codebase like pflib
 * isolating the python bindings to one file takes priority.
 *
 * The boost tutorial is a good reference for example code blocks
 * that we can almost straight copy. Note: the tutorial assumes
 * 'using namespace boost::python' which we will not do.
 * 
 * https://www.boost.org/doc/libs/1_75_0/libs/python/doc/html/tutorial/tutorial/exposing.html
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

namespace test {
/**
 * Want to make sure I can pass derived class through Python bindings
 */
class DerivedWBI : public WishboneInterface {
 public:
  // just doing an easy one
  void wb_reset() {
    std::cout << "I am in DerivedWBI!" << std::endl;
  }
}; //

class TestWBIUser {
 public:
  TestWBIUser(WishboneInterface* wb) : wb_{wb} {}
  void reset() { wb_->wb_reset(); }
 private:
  WishboneInterface* wb_;
};

}

}  // namespace pflib

/**
 * We define the name of the module to be 'pflib'
 * this needs to be the same as the name of the compiled dynamic library (everything before .so),
 * so cmake needs to be told to strip the default 'lib' prefix.
 */
BOOST_PYTHON_MODULE(pflib) {

  /**
   * I call this class "no_init" because it shouldn't be created in Python.
   * We don't bind any of its member functions because it shouldn't exist as an instance.
   */
  bp::class_<pflib::WishboneInterface>("WishboneInterface",bp::no_init);

  /**
   * Defining a new wishbone interface needs to clearly state that
   * it is a derived class in the python bindings as well.
   * This allows us to pass a Python instance of DerivedWBI to a function that
   * takes a WBI as an input.
   *
   * We don't need to bind any of its methods since all of its methods
   * are only used within a "WBI User" within the C++.
   */
  bp::class_<pflib::test::DerivedWBI, bp::bases<pflib::WishboneInterface>>("DerivedWBI");

  bp::class_<pflib::test::TestWBIUser>("TestWBIUser",bp::init<pflib::WishboneInterface*>())
    .def("reset",&pflib::test::TestWBIUser::reset)
  ;

  /**
   * Could combine set/get into a r/w python property
   *  https://www.boost.org/doc/libs/1_75_0/libs/python/doc/html/tutorial/tutorial/exposing.html#tutorial.exposing.class_properties
   */
  bp::class_<pflib::I2C>("I2C",bp::init<pflib::WishboneInterface*,int>())
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
