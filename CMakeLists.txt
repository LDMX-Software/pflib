# Set the minimum version of CMake that's required
cmake_minimum_required(VERSION 3.12)

# Set the project name
project(pflib
	DESCRIPTION "Pretty fine HGCROC configuration library."
	LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

# if we are in a conda environment,
# add the conda location to our search path
# and include its lib directory as a possible link area
if(DEFINED ENV{CONDA_PREFIX})
  set(CMAKE_PREFIX_PATH "$ENV{CONDA_PREFIX}")
  link_directories($ENV{CONDA_PREFIX}/lib)
endif()

find_package(Git)
add_custom_target(git_version
  ${CMAKE_COMMAND} -D SRC=${CMAKE_SOURCE_DIR}/src/pflib/version/cmake_version.h.in
                   -D DST=${CMAKE_BINARY_DIR}/include/version/cmake_version.h
                   -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
                   -P ${CMAKE_SOURCE_DIR}/GenerateVersionHeader.cmake
)
add_library(version SHARED src/pflib/version/Version.cxx)
target_include_directories(version PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>")
target_include_directories(version PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include/version/>"
  "$<INSTALL_INTERFACE:include>")
add_dependencies(version git_version)

# YAML->register value "compiler"
#   optional, build it if we can find the yaml parser we use
find_package(yaml-cpp REQUIRED)
message(STATUS "Found yaml-cpp ${yaml-cpp_VERSION}")
if (NOT TARGET yaml-cpp::yaml-cpp)
  add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
endif()

# Boost for CRC calculator, Test, Logging, and Python bindings
find_package(Boost COMPONENTS log unit_test_framework python REQUIRED)
# Python for Python bindings
find_package(Python3 COMPONENTS Interpreter Development)

if (DEFINED ENV{ROGUE_DIR})
  set(Rogue_DIR $ENV{ROGUE_DIR}/lib)
else()
  set(Rogue_DIR ${CMAKE_PREFIX_PATH}/lib)
endif()
find_package(Rogue CONFIG)
if (${Rogue_FOUND})
  list(REMOVE_ITEM ROGUE_LIBRARIES "PUBLIC")
  if (NOT TARGET Rogue::Rogue)
    add_library(Rogue::Rogue INTERFACE IMPORTED)
    set_target_properties(Rogue::Rogue PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${ROGUE_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${ROGUE_LIBRARIES}")
  endif()
endif()

# construct a register_maps library that the compiler can use
# to lookup registers given parameter names for a chip type
# hiding the ugly CMake code necessary to auto-gen the C++
# if a look-up-table changes and re-compile the library
add_subdirectory(register_maps)

add_library(utility SHARED
  src/pflib/utility/crc.cxx
  src/pflib/utility/ends_with.cxx
  src/pflib/utility/load_integer_csv.cxx
  src/pflib/utility/str_to_int.cxx
  src/pflib/utility/median.cxx
  src/pflib/utility/efficiency.cxx
)
target_include_directories(utility PUBLIC 
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>")
target_include_directories(utility SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})

add_library(logging SHARED src/pflib/logging/Logging.cxx)
target_include_directories(logging PUBLIC 
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(logging PUBLIC Boost::log)

add_library(packing SHARED
  src/pflib/packing/FileReader.cxx
  src/pflib/packing/BufferReader.cxx
  src/pflib/packing/Sample.cxx
  src/pflib/packing/DAQLinkFrame.cxx
  src/pflib/packing/TriggerLinkFrame.cxx
  src/pflib/packing/SingleROCEventPacket.cxx
  src/pflib/packing/ECONDEventPacket.cxx
  src/pflib/packing/MultiSampleECONDEventPacket.cxx
)
target_include_directories(packing PUBLIC 
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(packing PUBLIC utility)

# Build the pf library
#
#  src/pflib/FastControl.cxx
  
set(pflib_src
  src/pflib/I2C_Linux.cxx
  src/pflib/ROC.cxx
  src/pflib/ECON.cxx
  src/pflib/Compile.cxx
  src/pflib/HcalBackplane.cxx
  src/pflib/Target.cxx
  src/pflib/GPIO.cxx
  src/pflib/Elinks.cxx
  src/pflib/Parameters.cxx
  src/pflib/lpGBT.cxx
  src/pflib/lpgbt/lpGBT_ConfigTransport_I2C.cxx
  src/pflib/lpgbt/lpGBT_Registers.cxx
  src/pflib/lpgbt/lpGBT_Utility.cxx
  src/pflib/lpgbt/lpGBT_standard_configs.cxx
  src/pflib/lpgbt/I2C.cxx
  src/pflib/lpgbt/GPIO.cxx
  src/pflib/TargetFiberless.cxx
  src/pflib/GPIO_HcalHGCROCZCU.cxx
  src/pflib/FastControlCMS_MMap.cxx
  src/pflib/ECOND_Formatter.cxx
  src/pflib/zcu/UIO.cxx
  src/pflib/zcu/Elinks_zcu.cxx
  src/pflib/zcu/lpGBT_ICEC_ZCU_Simple.cxx
  src/pflib/zcu/zcu_elinks.cxx
  src/pflib/zcu/zcu_DAQ.cxx
  src/pflib/zcu/zcu_optolink.cxx
  src/pflib/zcu/HcalBackplaneZCU.cxx
  src/pflib/zcu/EcalSMMTarget.cxx
  src/pflib/Ecal.cxx
  src/pflib/Bias.cxx
)

if (${Rogue_FOUND})
  list(APPEND pflib_src
    src/pflib/bittware/bittware_axilite.cxx
    src/pflib/bittware/bittware_optolink.cxx
    src/pflib/bittware/bittware_elinks.cxx
    src/pflib/bittware/bittware_daq.cxx
    src/pflib/bittware/bittware_FastControl.cxx
    src/pflib/bittware/HcalBackplaneBittware.cxx
    src/pflib/bittware/EcalSMMTarget.cxx
    )
endif()

add_library(pflib SHARED ${pflib_src})
target_include_directories(pflib PUBLIC 
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>")
target_include_directories(pflib PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(pflib PUBLIC yaml-cpp::yaml-cpp packing version utility logging register_maps)
if (${Rogue_FOUND})
  target_link_libraries(pflib PUBLIC Rogue::Rogue)
endif()
add_dependencies(pflib direct_access lpgbt_regmap)

add_library(pypflib SHARED
  src/pflib/python/version.cxx
  src/pflib/python/logging.cxx
  src/pflib/python/packing.cxx
  src/pflib/python/bindings.cxx)
target_link_libraries(pypflib PUBLIC pflib Boost::python Python3::Python)

add_library(menu SHARED src/pflib/menu/Menu.cc)
target_link_libraries(menu PUBLIC readline pflib logging)
target_include_directories(menu PUBLIC 
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>")

add_executable(
  test-pflib
  test/main.cxx
  test/compile.cxx
  test/decoding.cxx
  test/utility.cxx
  test/parameters.cxx
)
target_link_libraries(test-pflib PRIVATE Boost::unit_test_framework pflib packing)

add_executable(
  test-pflib-on-zcu
  test/on_zcu/main.cxx
  test/on_zcu/hgcroc_connection.cxx
  test/on_zcu/test_parameters.cxx
  test/on_zcu/elinks_aligned.cxx
  test/on_zcu/read_bias.cxx
)
target_link_libraries(test-pflib-on-zcu PRIVATE Boost::unit_test_framework pflib packing)

find_package(nlohmann_json 3.10.5 REQUIRED)
file(GLOB_RECURSE pftool_sources LIST_DIRECTORIES FALSE app/tool/*.cxx)
add_executable(pftool ${pftool_sources})
target_link_libraries(pftool PRIVATE pflib menu nlohmann_json::nlohmann_json)
if (${Rogue_FOUND})
  target_compile_definitions(pftool PRIVATE USE_ROGUE)
endif()

add_executable(pflpgbt app/lpgbt/main.cxx app/lpgbt/lpgbt_mezz_tester.cc)
if (${Rogue_FOUND})
  target_compile_definitions(pflpgbt PRIVATE USE_ROGUE)
endif()
target_link_libraries(pflpgbt PRIVATE pflib menu logging)

# don't install test-menu executable! just for Menu developments
file(GLOB_RECURSE test_menu_sources LIST_DIRECTORIES FALSE test/test_menu/*.cxx)
add_executable(test-menu ${test_menu_sources})
target_link_libraries(test-menu PRIVATE pflib menu)

add_executable(pfdecoder app/pfdecoder.cxx)
target_link_libraries(pfdecoder PRIVATE pflib)

add_executable(econd-decoder app/econd_decoder.cxx)
target_link_libraries(econd-decoder PRIVATE pflib)

if (${Rogue_FOUND})
  add_executable(rogue-decoder app/rogue_decoder.cxx)
  target_link_libraries(rogue-decoder PUBLIC pflib Rogue::Rogue)
endif()

add_executable(pfcompile app/pfcompile.cxx)
target_link_libraries(pfcompile pflib)

add_executable(pfdecompile app/pfdecompile.cxx)
target_link_libraries(pfdecompile pflib)

add_executable(pfdefaults app/pfdefaults.cxx)
target_link_libraries(pfdefaults pflib)

# add pflib prefix to all our pflib libraries
set_target_properties(pflib menu packing logging version utility register_maps PROPERTIES PREFIX "libpflib_")
# remove 'lib' prefix from pypflib so we can 'import pypflib' in Python
set_target_properties(pypflib PROPERTIES PREFIX "")
install(PROGRAMS app/rogue-decoder.py DESTINATION bin)
install(TARGETS pflib packing logging version utility register_maps pypflib pftool pfdecoder pfdecompile pfcompile pfdefaults
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
