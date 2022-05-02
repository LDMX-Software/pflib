# Set the minimum version of CMake that's required
cmake_minimum_required(VERSION 3.12)

execute_process(
  COMMAND git describe --tags
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  RESULT_VARIABLE git_rc
  OUTPUT_VARIABLE git_describe)
if (NOT ${git_rc} EQUAL 0)
  set(pflib_vers "0.0")
  message(WARNING "System cannot deduce git tag, this pflib built is un-versioned.")
else()
  string(REGEX REPLACE "^v" "" git_describe ${git_describe})
  string(REGEX REPLACE "-.*$" "" git_describe ${git_describe})
  string(REGEX REPLACE "\n$" "" git_describe ${git_describe})
  set(pflib_vers ${git_describe})
endif()
message(STATUS "Deduced pflib version ${pflib_vers}")

# Set the project name
project(pflib
	VERSION ${pflib_vers}
	DESCRIPTION "Polarfire configuration library."
	LANGUAGES CXX)

# YAML->register value "compiler"
#   optional, build it if we can find the yaml parser we use
find_package(yaml-cpp REQUIRED)
message(STATUS "Found yaml-cpp ${yaml-cpp_VERSION}")

# Build the polarfire library
add_library(pflib SHARED 
  src/pflib/Backend.cxx
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
  src/pflib/PolarfireTarget.cxx
  src/pflib/decoding/RocPacket.cxx
  src/pflib/decoding/PolarfirePacket.cxx
  src/pflib/decoding/SuperPacket.cxx
)
target_include_directories(pflib PUBLIC 
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(pflib PUBLIC yaml-cpp)

add_executable(pftool
  tool/pftool.cc
  tool/Menu.cc
  tool/Rcfile.cc
  tool/pftool_tasks.cc
  tool/pftool_roc.cc
  tool/pftool_daq.cc
  tool/pftool_expert.cc
  tool/pftool_elinks.cc
  tool/pftool_headers.cc
  tool/pftool_fastcontrol.cc
  tool/pftool_bias.cc)
target_link_libraries(pftool PRIVATE pflib readline)

# don't install test-menu executable! just for Menu developments
add_executable(test-menu tool/test_menu.cxx tool/Menu.cc)
target_link_libraries(test-menu PRIVATE pflib readline)

add_executable(pfdecoder tool/pfdecoder.cc)

add_executable(pfcompile tool/pfcompile.cxx)
target_link_libraries(pfcompile pflib)

add_executable(pfdecompile tool/pfdecompile.cxx)
target_link_libraries(pfdecompile pflib)

add_executable(pfdefaults tool/pfdefaults.cxx)
target_link_libraries(pfdefaults pflib)

add_executable(configure-detector 
  tool/detector/DetectorConfiguration.cxx 
  tool/detector/configure-detector.cxx)
target_link_libraries(configure-detector PRIVATE pflib)
# Now determine which method of interaction to use for communicating with the polarfire
#   - if neither are available, fatal cmake error - this library is useless without one

# Rogue
if (DEFINED ENV{ROGUE_DIR})
   set(Rogue_DIR $ENV{ROGUE_DIR}/lib)
else()
   set(Rogue_DIR ${CMAKE_PREFIX_PATH}/lib)
endif()
# issues its own "found Rogue version ___" message
find_package(Rogue CONFIG)

if (Rogue_FOUND)
  # the libraries parameter includes the PUBLIC keyword annoyingly
  list(REMOVE_ITEM ROGUE_LIBRARIES "PUBLIC")
  # define source files this submodule uses
  add_library(rogue SHARED src/pflib/rogue/RogueWishboneInterface.cxx)
  # attach rogue as a dependency
  target_link_libraries(rogue PUBLIC pflib ${ROGUE_LIBRARIES})
  target_include_directories(rogue PUBLIC ${ROGUE_INCLUDE_DIRS})
  # let executables that need it know that rogue is available
  target_compile_definitions(pftool PRIVATE PFTOOL_ROGUE=1)
  target_compile_definitions(configure-detector PRIVATE PFTOOL_ROGUE=1)
  # link submodule lib to executables that need it
  target_link_libraries(pftool PRIVATE rogue)
  target_link_libraries(configure-detector PRIVATE rogue)
  list(APPEND comm_libs rogue)
else()
  message(STATUS "Unable to find Rogue, will not compile rogue submodule of pflib.")
endif()

# uHal
if (DEFINED ENV{UHAL_DIR})
  set(Uhal_DIR $ENV{UHAL_DIR})
else()
  set(Uhal_DIR /opt/cactus)
endif()

if(EXISTS ${Uhal_DIR}/include/uhal/version.hpp)
  # we found uHal!
  set(Uhal_FOUND YES)
  # deduce its version
  file(STRINGS ${Uhal_DIR}/include/uhal/version.hpp version_lines REGEX "^#define UHAL_VERSION_(.*)$")
  foreach(version_line ${version_lines})
    string(REGEX REPLACE "^#define " "" trimmed ${version_line})
    message(STATUS ${trimmed})
  endforeach()  
  #message(STATUS "Uhal version: ${UHAL_VERSION_MAJOR}.${UHAL_VERSION_MINOR}.${UHAL_VERSION_PATCH}")
  # define source files this submodule uses
  add_library(uhal SHARED src/pflib/uhal/uhalWishboneInterface.cxx)
  # attach uHal as a dependency
  target_link_libraries(uhal PUBLIC pflib "${Uhal_DIR}/lib/libcactus_uhal_uhal.so")
  target_include_directories(uhal PUBLIC "${Uhal_DIR}/include")
  # let executables that need it know that uHal is available
  target_compile_definitions(pftool PRIVATE PFTOOL_UHAL=1)
  target_compile_definitions(configure-detector PRIVATE PFTOOL_UHAL=1)
  # link submodule lib to executables that need it
  target_link_libraries(pftool PRIVATE uhal)
  target_link_libraries(configure-detector PRIVATE uhal)
  list(APPEND comm_libs uhal)
else()
  set(Uhal_FOUND NO)
  message(STATUS "Unable to find uHal, will not compile uhal submodule of pflib.")
endif()

if (NOT Rogue_FOUND AND NOT Uhal_FOUND)
  message(WARNING "Neither Rogue or Uhal were found. At least one is necessary for a functional polarfire interaction library.")
endif()

set_target_properties(pflib ${comm_libs} PROPERTIES PREFIX "libpflib_")

install(TARGETS pflib ${comm_libs} 
  pftool pfdecoder pfdecompile pfcompile pfdefaults configure-detector
  EXPORT pflibTargets 
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
  )
install(DIRECTORY include/ DESTINATION include/ FILES_MATCHING PATTERN "*.h")
install(EXPORT pflibTargets
  FILE pflibTargets.cmake
  NAMESPACE pflib::
  DESTINATION lib/cmake/pflib
  )

include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/pflibConfig.cmake"
  INSTALL_DESTINATION lib/cmake/pflib
  )
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/pflibConfigVersion.cmake"
  COMPATIBILITY AnyNewerVersion)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/pflibConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/pflibConfigVersion.cmake"
  DESTINATION lib/cmake/pflib
  )
