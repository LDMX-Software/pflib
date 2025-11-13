#pragma once

// the boost::bind library was updated and these updates
// had not made it into Boost.Python for the version we are using
#define BOOST_BIND_GLOBAL_PLACEHOLDERS

#include <boost/python.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/numpy.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/str.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python/tuple.hpp>

/// short name for the boost::python namespace
namespace bp = boost::python;

/**
 * Initialize a submodule within the parent module named 'pypflib'.
 *
 * This should go at the beginning of a function that makes Python
 * stuff that should go into a submodule.
 *
 * @param[in] name name of submodule
 */
#define BOOST_PYTHON_SUBMODULE(name)                                     \
  bp::object submodule(                                                  \
      bp::handle<>(bp::borrowed(PyImport_AddModule("pypflib." #name)))); \
  bp::scope().attr(#name) = submodule;                                   \
  bp::scope io_scope = submodule;                                        \
