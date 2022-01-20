# Set the minimum version of CMake that's required
cmake_minimum_required(VERSION 3.12)

# Set the project name
project(pflib
	VERSION 0.1
	DESCRIPTION "Polarfire configuration library."
	LANGUAGES CXX)

# Build the polarfire library
file(GLOB pflib_src CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/pflib/[a-zA-Z]*.cxx)
add_library(pflib SHARED ${pflib_src})
target_include_directories(pflib PUBLIC ${PROJECT_SOURCE_DIR}/include)
install(TARGETS pflib LIBRARY DESTINATION lib)

setup_rogue()

if (Rogue_FOUND)
  file(GLOB pflib_rogue_src CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/pflib/rogue/[a-zA-Z]*.cxx)
  add_library(pflib_rogue SHARED ${pflib_rogue_src})
  target_link_libraries(pflib_rogue PUBLIC pflib Rogue::Rogue)
  install(TARGETS pflib_rogue LIBRARY DESTINATION lib)
endif()

# Search for Boost + Python
#   shamelessly stolen from slaclab/rogue central CMakeLists.txt
#   need python includes and libraries for compiling boost python bindings
find_package(PythonInterp 3 QUIET REQUIRED)
find_package(PythonLibs 3 QUIET REQUIRED)
if(NOT TARGET Python::Interface)
  add_library(Python::Interface INTERFACE IMPORTED GLOBAL)
  set_target_properties(Python::Interface PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${PYTHON_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "${PYTHON_LIBRARIES}")
endif()

if (NOT Boost_FOUND)
  set(Boost_USE_MULTITHREADED ON)
  set(Boost_NO_BOOST_CMAKE ON)
  
  # Hint for boost on anaconda
  if (DEFINED ENV{CONDA_PREFIX})
     set(BOOST_ROOT $ENV{CONDA_PREFIX})
  # custom path
  elseif (DEFINED ENV{BOOST_PATH})
     set(BOOST_ROOT $ENV{BOOST_PATH})
  endif()
  
  # Boost.Python is pretty new and the target has changed
  #   naming conventions several times as it was developed
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
      if (NOT ${target_opt} STREQUAL "python")
        # make an alias so we can use the simple target name
        add_library(Boost::python ALIAS Boost::${target_opt})
      endif()
      # we found it, leave
      break()
    endif()
  endforeach()
  
  if (NOT Boost_FOUND)
    # nothing worked
    message("")
    message(FATAL_ERROR "Failed to find boost libraries!")
  endif()
endif()

if (Rogue_FOUND)
  add_library(pflib_python SHARED ${PROJECT_SOURCE_DIR}/src/pflib/python/Bindings.cxx)
  target_link_libraries(pflib_python PUBLIC pflib pflib_rogue Boost::python Python::Interface)
  install(TARGETS pflib_python LIBRARY DESTINATION lib)

  # remove 'lib' prefix from compiled shared library
  #   this enables us to do 'import pflib' instead of 'import libpflib'
  set_target_properties(pflib_python PROPERTIES PREFIX "")
endif()
