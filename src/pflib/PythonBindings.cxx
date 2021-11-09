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

#include <iostream>
#include <boost/python.hpp>
namespace bp = boost::python;

#include "pflib/Exception.h"
#include "pflib/Hcal.h"
#include "pflib/Elinks.h"
#include "pflib/Bias.h"
#include "pflib/DAQ.h"
#include "pflib/FastControl.h"
#include "pflib/GPIO.h"
#include "pflib/I2C.h"
#include "pflib/ROC.h"
#include "pflib/WishboneInterface.h"
#include "pflib/rogue/RogueWishboneInterface.h"

namespace pflib {

/**
 * Exception translation.
 * 'All C++ exceptions must be caught at the boundary with Python code'
 *    - Boost.Python docs
 *  They do provide a method of translating the message to a python one.
 */
void ExceptionTranslator(Exception const& e) {
  std::string msg = "[" + e.name() + "] " + e.message() + " in " + e.module();
  PyErr_SetString(PyExc_UserWarning, msg.c_str());
}

}  // namespace pflib

/**
 * We define the name of the module to be 'pflib'
 * this needs to be the same as the name of the compiled dynamic library (everything before .so),
 * so cmake needs to be told to strip the default 'lib' prefix.
 */
BOOST_PYTHON_MODULE(pflib) {

  bp::register_exception_translator<pflib::Exception>(pflib::ExceptionTranslator);

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
  bp::class_<pflib::rogue::RogueWishboneInterface, 
    bp::bases<pflib::WishboneInterface>>("RogueWishboneInterface", bp::init<std::string,int>());

  bp::class_<pflib::I2C>("I2C",bp::init<pflib::WishboneInterface*,bp::optional<int>>())
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

  bp::class_<pflib::GPIO>("GPIO",bp::init<pflib::WishboneInterface*,bp::optional<int>>())
    .def("getGPOcount", &pflib::GPIO::getGPOcount)
    .def("getGPIcount", &pflib::GPIO::getGPIcount)
    .def("getGPI", &pflib::GPIO::getGPI_single)
    .def("getGPI", &pflib::GPIO::getGPI_all)
    .def("setGPO", &pflib::GPIO::setGPO_single)
    .def("setGPO", &pflib::GPIO::setGPO_all)
    .def("getGPO", &pflib::GPIO::getGPO)
  ;

  bp::class_<pflib::FastControl>("FastControl",bp::init<pflib::WishboneInterface*,bp::optional<int>>())
    .def("getCmdCounters", &pflib::FastControl::getCmdCounters)
    .def("getErrorCounters", &pflib::FastControl::getErrorCounters)
    .def("resetCounters", &pflib::FastControl::resetCounters)
    .def("resetTransmitter", &pflib::FastControl::resetTransmitter)
    .def("setupMultisample", &pflib::FastControl::setupMultisample)
    .def("getMultisampleSetup", &pflib::FastControl::getMultisampleSetup)
  ;

  bp::class_<pflib::DAQ>("DAQ",bp::init<pflib::WishboneInterface*>())
    .def("reset", &pflib::DAQ::reset)
    .def("getHeaderOccupancy", &pflib::DAQ::getHeaderOccupancy)
    .def("setIds", &pflib::DAQ::setIds)
    .def("getFPGAid", &pflib::DAQ::getFPGAid)
    .def("setupLink", &pflib::DAQ::setupLink)
    .def("getLinkSetup", &pflib::DAQ::getLinkSetup)
    .def("bufferStatus", &pflib::DAQ::bufferStatus)
    .def("enable", &pflib::DAQ::enable)
    .def("enabled", &pflib::DAQ::enabled)
    .def("nlinks", &pflib::DAQ::nlinks)
  ;

  bp::class_<pflib::Elinks>("Elinks",bp::init<pflib::WishboneInterface*, bp::optional<int>>())
    .def("spy", &pflib::Elinks::spy)
    .def("setBitslip", &pflib::Elinks::setBitslip)
    .def("setBitslipAuto", &pflib::Elinks::setBitslipAuto)
    .def("isBitslipAuto", &pflib::Elinks::isBitslipAuto)
    .def("getBitslip", &pflib::Elinks::getBitslip)
    .def("getStatusRaw", &pflib::Elinks::getStatusRaw)
    .def("clearErrorCounters", &pflib::Elinks::clearErrorCounters)
    .def("readCounters", &pflib::Elinks::readCounters)
    .def("setupBigspy", &pflib::Elinks::setupBigspy)
    .def("getBigspySetup", &pflib::Elinks::getBigspySetup)
    .def("bigspyDone", &pflib::Elinks::bigspyDone)
    .def("bigspy", &pflib::Elinks::bigspy)
    .def("resetHard", &pflib::Elinks::resetHard)
    .def("scanAlign", &pflib::Elinks::scanAlign)
    .def("setDelay", &pflib::Elinks::setDelay)
  ;

  bp::class_<pflib::ROC>("ROC",bp::init<pflib::I2C&,bp::optional<int>>())
    .def("readPage", &pflib::ROC::readPage)
    .def("setValue", &pflib::ROC::setValue)
    .def("getChannelParameters", &pflib::ROC::getChannelParameters)
    .def("setChannelParameters", &pflib::ROC::setChannelParameters)
  ;

  bp::class_<pflib::MAX5825>("MAX5825",bp::init<pflib::I2C&,uint8_t,bp::optional<int>>())
    .def_readonly("WDOG",&pflib::MAX5825::WDOG)
    .def_readonly("RETURNn",&pflib::MAX5825::RETURNn)
    .def_readonly("CODEn",&pflib::MAX5825::CODEn)
    .def_readonly("LOADn",&pflib::MAX5825::LOADn)
    .def_readonly("CODEn_LOADALL",&pflib::MAX5825::CODEn_LOADALL)
    .def_readonly("CODEn_LOADn",&pflib::MAX5825::CODEn_LOADn)
    .def_readonly("REFn",&pflib::MAX5825::REFn)
    .def_readonly("POWERn",&pflib::MAX5825::POWERn)
    .def("get", &pflib::MAX5825::get)
    .def("set", &pflib::MAX5825::set)
    .def("getByDAC", &pflib::MAX5825::getByDAC)
    .def("setByDAC", &pflib::MAX5825::setByDAC)
    .def("setRefVoltage", &pflib::MAX5825::setRefVoltage)
  ;

  // choosing to not bind addresss variables because they should be used by end user
  bp::class_<pflib::Bias>("Bias",bp::init<pflib::I2C&,bp::optional<int>>())
    .def("initialize", &pflib::Bias::initialize)
    .def("cmdLED", &pflib::Bias::cmdLED)
    .def("cmdSiPM", &pflib::Bias::cmdSiPM)
    .def("setLED", &pflib::Bias::setLED)
    .def("setSiPM", &pflib::Bias::setSiPM)
  ;

  bp::class_<pflib::Hcal>("Hcal",bp::init<pflib::WishboneInterface*>())
    .def("roc", &pflib::Hcal::roc)
    .def("bias", &pflib::Hcal::bias)
    .def("daq", &pflib::Hcal::daq, bp::return_internal_reference<>())
    .def("hardResetROCs", &pflib::Hcal::hardResetROCs)
    .def("softResetROC", &pflib::Hcal::softResetROC)
    .def("resyncLoadROC", &pflib::Hcal::resyncLoadROC)
  ;

};
