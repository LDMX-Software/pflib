/**
 * Definition of python module for polarfire library bindings
 *
 * Following structure laid out in slaclab/rogue project.
 * We use static public member functions for defining the python
 * bindings and call those member functions within this macro so
 * that they all are within the 'pflib' python module.
 *
 * Adding submodules can be done creating a new module object
 * and defining that as the scope _and then_ calling the static methods.
 *
 * This can be seen in 
 *  https://github.com/slaclab/rogue/blob/master/src/rogue/interfaces/module.cpp
 */

#include <boost/python.hpp>

#include "pflib/Bias.h"
#include "pflib/WishboneInterface.h"

BOOST_PYTHON_MODULE(pflib) {
  pflib::WishboneInterface::declare_python();
  pflib::MAX5825::declare_python();
};
