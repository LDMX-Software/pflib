# Set the minimum version of CMake that's required
cmake_minimum_required(VERSION 3.12)

# Set the project name
project(pflib
	VERSION 0.1
	DESCRIPTION "Polarfire configuration library."
	LANGUAGES CXX)

# Search for Boost + Python
#   shamelessly stolen from slaclab/rogue
#   https://github.com/slaclab/rogue/blob/370431eba80b39b62af48ff54b6602c828f7e215/CMakeLists.txt#L44
find_package(PythonInterp 3 QUIET REQUIRED)
find_package(PythonLibs 3 QUIET REQUIRED)

# Find Numpy
execute_process(
   COMMAND python3 -c "import numpy; print(numpy.get_include(), end='')"
   RESULT_VARIABLE Python3_NumPy_RESULT
   OUTPUT_VARIABLE Python3_NumPy_INCLUDE_DIRS
   ERROR_QUIET
   OUTPUT_STRIP_TRAILING_WHITESPACE)

if (Python3_NumPy_RESULT)
  message("")
  message(FATAL_ERROR "Failed to find numpy includes!")
endif()

set(Boost_USE_MULTITHREADED ON)
set(Boost_NO_BOOST_CMAKE ON)

# Hint for boost on anaconda
if (DEFINED ENV{CONDA_PREFIX})
   set(BOOST_ROOT $ENV{CONDA_PREFIX})
# custom path
elseif (DEFINED ENV{BOOST_PATH})
   set(BOOST_ROOT $ENV{BOOST_PATH})
endif()

set(BOOSTPY_TARGET_OPTIONS 
  "python${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}"
  "python${PYTHON_VERSION_MAJOR}"
  "python"
  "python-py${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}"
  "python3-py${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}")
foreach(target_opt ${BOOSTPY_TARGET_OPTIONS})
  message("looking for libboost_${target_opt}")
  find_package(Boost 1.58 QUIET COMPONENTS ${target_opt})
  if (Boost_FOUND)
    add_library(Boost::python ALIAS Boost::${target_opt})
    break()
  endif()
endforeach()

# Nothing worked
if (NOT Boost_FOUND)
  message("")
  message(FATAL_ERROR "Failed to find boost libraries!")
endif()

# Build the emulator library
setup_library(module pflib dependencies Boost::python)

