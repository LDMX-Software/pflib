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

setup_rogue()

add_library(pflib_rogue SHARED src/pflib/rogue/RogueWishboneInterface.cxx)
target_link_libraries(pflib_rogue PUBLIC pflib Rogue::Rogue)
add_library(pflib::rogue ALIAS pflib_rogue)

add_executable(pftool tool/pftool.cc)
target_link_libraries(pftool PRIVATE pflib pflib_rogue readline)

add_executable(pfcompile tool/pfcompile.cxx)
target_link_libraries(pfcompile pflib pflib_rogue)

add_executable(pfdecompile tool/pfdecompile.cxx)
target_link_libraries(pfdecompile pflib pflib_rogue)

install(TARGETS pftool pfcompile pfdecompile DESTINATION bin)
install(TARGETS pflib pflib_rogue LIBRARY DESTINATION lib)
