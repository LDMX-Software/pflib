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

add_executable(pftool tool/pftool.cc)
target_link_libraries(pftool PRIVATE pflib readline)

add_executable(pfdecoder tool/pfdecoder.cc)

add_executable(pfcompile tool/pfcompile.cxx)
target_link_libraries(pfcompile pflib)

add_executable(pfdecompile tool/pfdecompile.cxx)
target_link_libraries(pfdecompile pflib)

install(TARGETS pftool pfdecoder pfcompile pfdecompile DESTINATION bin)

# Now determine which method of interaction to use for communicating with the polarfire
#   - if neither are available, fatal cmake error - this library is useless without one
setup_rogue()
if (Rogue_FOUND)
  add_library(pflib_rogue SHARED src/pflib/rogue/RogueWishboneInterface.cxx)
  target_link_libraries(pflib_rogue PUBLIC pflib Rogue::Rogue)
  add_library(pflib::rogue ALIAS pflib_rogue)
  install(TARGETS pflib_rogue LIBRARY DESTINATION lib)
  target_compile_definitions(pftool PRIVATE PFTOOL_ROGUE=1)
  target_link_libraries(pftool PRIVATE pflib_rogue)
endif()

setup_uhal()
if (Uhal_FOUND)
  add_library(pflib_uhal SHARED src/pflib/uhal/uhalWishboneInterface.cxx)
  target_link_libraries(pflib_uhal PUBLIC pflib Uhal::Uhal)
  add_library(pflib::uhal ALIAS pflib_uhal)
  target_compile_definitions(pftool PRIVATE PFTOOL_UHAL=1)
  target_link_libraries(pftool PRIVATE pflib_uhal)
endif()

if (NOT Rogue_FOUND AND NOT Uhal_FOUND)
  message(FATAL_ERROR "Neither Rogue or Uhal were found. At least one is necessary for polarfire interaction library.")
endif()
