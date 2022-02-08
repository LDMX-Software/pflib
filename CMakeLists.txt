# Set the minimum version of CMake that's required
cmake_minimum_required(VERSION 3.12)

# Set the project name
project(pflib
	VERSION 3.0
	DESCRIPTION "Polarfire configuration library."
	LANGUAGES CXX)

# YAML->register value "compiler"
#   optional, build it if we can find the yaml parser we use
find_package(yaml-cpp REQUIRED)

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
  src/pflib/Compile.cxx
  src/pflib/PolarfireTarget.cxx)
target_include_directories(pflib PUBLIC include)
target_link_libraries(pflib PUBLIC yaml-cpp)
add_library(pflib::pflib ALIAS pflib)
install(TARGETS pflib LIBRARY DESTINATION lib)

add_executable(pftool tool/pftool.cc tool/Menu.cc tool/Rcfile.cc)
target_link_libraries(pftool PRIVATE pflib readline)

add_executable(pfdecoder tool/pfdecoder.cc)

add_executable(pfcompile tool/pfcompile.cxx)
target_link_libraries(pfcompile pflib)

add_executable(pfdecompile tool/pfdecompile.cxx)
target_link_libraries(pfdecompile pflib)

install(TARGETS pftool pfdecoder pfcompile pfdecompile DESTINATION bin)

# Now determine which method of interaction to use for communicating with the polarfire
#   - if neither are available, fatal cmake error - this library is useless without one

# Rogue
if (DEFINED ENV{ROGUE_DIR})
   set(Rogue_DIR $ENV{ROGUE_DIR}/lib)
else()
   set(Rogue_DIR ${CMAKE_PREFIX_PATH}/lib)
endif()
find_package(Rogue CONFIG)

if (Rogue_FOUND)
  # the libraries parameter includes the PUBLIC keyword annoyingly
  list(REMOVE_ITEM ROGUE_LIBRARIES "PUBLIC")
  # define source files this submodule uses
  add_library(pflib_rogue SHARED src/pflib/rogue/RogueWishboneInterface.cxx)
  # attach dependencies of this submodule
  target_link_libraries(pflib_rogue PUBLIC pflib ${ROGUE_LIBRARIES})
  target_include_directories(pflib_rogue PUBLIC ${ROGUE_INCLUDE_DIRS})
  # add the alias to act like we were imported (even if we aren't)
  add_library(pflib::rogue ALIAS pflib_rogue)
  # install rogue submodule
  install(TARGETS pflib_rogue LIBRARY DESTINATION lib)
  # let pftool know that rogue is available
  target_compile_definitions(pftool PRIVATE PFTOOL_ROGUE=1)
  # link submodule lib to pftool
  target_link_libraries(pftool PRIVATE pflib_rogue)
else()
  message(STATUS "Unable to find Rogue, will not compile rogue submodule of pflib.")
endif()

# uHal
if (DEFINED ENV{UHAL_DIR})
  set(Uhal_DIR $ENV{UHAL_DIR})
else()
  set(Uhal_DIR /opt/cactus)
endif()

if(EXISTS ${Uhal_DIR}/include/uhal/Node.hpp)
  # we found uHal!
  set(Uhal_FOUND YES)
  # define source files this submodule uses
  add_library(pflib_uhal SHARED src/pflib/uhal/uhalWishboneInterface.cxx)
  # link us to the uHal library
  target_link_libraries(pflib_uhal PUBLIC pflib "${Uhal_DIR}/lib/libcactus_uhal_uhal.so")
  # include the uHal header
  target_include_directories(pflib_uhal PUBLIC "${Uhal_DIR}/include")
  # add lias to act like we were imported (even if we aren't)
  add_library(pflib::uhal ALIAS pflib_uhal)
  # let pftool know that uHal is available
  target_compile_definitions(pftool PRIVATE PFTOOL_UHAL=1)
  # link submodule lib to pftool
  target_link_libraries(pftool PRIVATE pflib_uhal)
else()
  set(Uhal_FOUND NO)
  message(STATUS "Unable to find uHal, will not compile uhal submodule of pflib.")
endif()

if (NOT Rogue_FOUND AND NOT Uhal_FOUND)
  message(FATAL_ERROR "Neither Rogue or Uhal were found. At least one is necessary for polarfire interaction library.")
endif()
