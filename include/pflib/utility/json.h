/**
 * @file json.h
 *
 * In order to only compile Boost.JSON once and allow it
 * to link across multiple libraries, we include the declaration
 * header here for downstream libraries to use and then include
 * the source file in the definition file.
 */
#pragma once

#include <boost/json.hpp>
