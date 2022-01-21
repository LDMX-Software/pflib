# Set the minimum version of CMake that's required
cmake_minimum_required(VERSION 3.12)

# Set the project name
project(pflib
	VERSION 0.1
	DESCRIPTION "Polarfire configuration library."
	LANGUAGES CXX)

# YAML->register value "compiler"
#   optional, build it if we can find the yaml parser we use
find_package(yaml-cpp REQUIRED)
if (yaml-cpp_FOUND)
  # now we are able to compile the yaml->csv "compiler" of polarfire/hgcroc settings
  get_target_property(libs yaml-cpp INTERFACE_LINK_LIBRARIES)
  message("yaml-cpp libs: ${libs}")
  get_target_property(head yaml-cpp INTERFACE_INCLUDE_DIRECTORIES)
  message("yaml-cpp inc : ${head}")
  add_library(pflib_compile SHARED src/pflib/compile/Compiler.cxx)
  target_link_libraries(pflib_compile PUBLIC yaml-cpp)
  target_include_directories(pflib_compile PUBLIC include)

  add_executable(pfcompile tool/pfcompile.cxx)
  target_link_libraries(pfcompile pflib_compile)

  set(PFLIB_CAN_COMPILE_YAML "YES")

  install(TARGETS pfcompile DESTINATION bin)
  install(TARGETS pflib_compile LIBRARY DESTINATION lib)
else()
  message(WARNING "Unable to find yaml-cpp. pflib will not be able to 'compile' yaml files into HGC ROC settings that can be loaded onto the chip.")
endif()
configure_file(${PROJECT_SOURCE_DIR}/include/pflib/compile/CanCompile.h.in
               ${PROJECT_SOURCE_DIR}/include/pflib/compile/CanCompile.h)

# Build the polarfire library
add_library(pflib SHARED 
  src/pflib/Bias.cxx
  src/pflib/DAQ.cxx
  src/pflib/Elinks.cxx
  src/pflib/FastControl.cxx
  src/pflib/GPIO.cxx
  src/pflib/Hcal.cxx
  src/pflib/I2C.cxx
  src/pflib/ROC.cxx
  src/pflib/WishboneTarget.cxx
  src/pflib/PolarfireTarget.cxx)
target_include_directories(pflib PUBLIC include)
add_library(pflib::pflib ALIAS pflib)

setup_rogue()

add_library(pflib_rogue SHARED src/pflib/rogue/RogueWishboneInterface.cxx)
target_link_libraries(pflib_rogue PUBLIC pflib Rogue::Rogue)
add_library(pflib::rogue ALIAS pflib_rogue)

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

add_library(pflib_python SHARED src/pflib/python/Bindings.cxx)
target_link_libraries(pflib_python PUBLIC pflib pflib_rogue Boost::python Python::Interface)

# remove 'lib' prefix from compiled shared library
#   this enables us to do 'import pflib' instead of 'import libpflib'
set_target_properties(pflib_python PROPERTIES PREFIX "")
add_library(pflib::python ALIAS pflib_python)

add_executable(pftool tool/pftool.cc)
target_link_libraries(pftool PRIVATE pflib pflib_rogue readline)

install(TARGETS pftool DESTINATION bin)
install(TARGETS pflib pflib_rogue pflib_python LIBRARY DESTINATION lib)
