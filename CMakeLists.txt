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
	DESCRIPTION "Pretty fine HGCROC configuration library."
	LANGUAGES CXX)

# YAML->register value "compiler"
#   optional, build it if we can find the yaml parser we use
find_package(yaml-cpp REQUIRED)
message(STATUS "Found yaml-cpp ${yaml-cpp_VERSION}")

# Build the pf library
#
#  src/pflib/Bias.cxx
#  src/pflib/FastControl.cxx
  
add_library(pflib SHARED 
  src/pflib/I2C_Linux.cxx
  src/pflib/ROC.cxx
  src/pflib/Compile.cxx
  src/pflib/Hcal.cxx
  src/pflib/Target.cxx
  src/pflib/TargetFiberless.cxx
  src/pflib/GPIO.cxx
  src/pflib/GPIO_HcalHGCROCZCU.cxx
  src/pflib/FastControlCMS_MMap.cxx
  src/pflib/zcu/UIO.cxx
)
target_include_directories(pflib PUBLIC 
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(pflib PUBLIC yaml-cpp)

add_executable(pftool tool/pftool.cc tool/Menu.cc tool/Rcfile.cc)
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
  # let pftool know that rogue is available
  target_compile_definitions(pftool PRIVATE PFTOOL_ROGUE=1)
  # link submodule lib to pftool
  target_link_libraries(pftool PRIVATE rogue)
  list(APPEND comm_libs rogue)
else()
  message(STATUS "Unable to find Rogue, will not compile rogue submodule of pflib.")
endif()

#if (NOT Rogue_FOUND AND NOT Uhal_FOUND)
#  message(WARNING "Neither Rogue or Uhal were found. At least one is necessary for a functional polarfire interaction library.")
#endif()

set_target_properties(pflib ${comm_libs} PROPERTIES PREFIX "libpflib_")

install(TARGETS pflib ${comm_libs} pftool pfdecoder pfdecompile pfcompile pfdefaults
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
