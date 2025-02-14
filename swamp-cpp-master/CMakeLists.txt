cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project( swamp )

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-std=c++17 HAVE_FLAG_STD_CXX17)
if(HAVE_FLAG_STD_CXX17)
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17" )
  message(STATUS "Will use c++17")
else()
  message( FATAL_ERROR "C++ 17 or higher is required")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lyaml-cpp -lrt -lpthread -lhiredis -lredis++ -Wall -Wextra -Wpedantic -fPIC -O3")

set(DESTINATION "/opt/${PROJECT_NAME}")

list( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )

find_package( Boost COMPONENTS thread system filesystem timer REQUIRED )

message(STATUS "Boost_INCLUDE_DIRS = ${Boost_INCLUDE_DIRS}")
include_directories( SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/swamp/include ${Boost_INCLUDE_DIRS} )
link_directories( ${EXTERN_BOOST_LIB_PREFIX} )

file( GLOB sources ${PROJECT_SOURCE_DIR}/swamp/src/*.cpp )
file( GLOB headers ${PROJECT_SOURCE_DIR}/swamp/include/*.hpp )

find_package( CACTUS REQUIRED )
find_package( EMP REQUIRED )
# find_package( spdlog CONFIG REQUIRED )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include  ${UHAL_LOG_INCLUDE_PREFIX} ${UHAL_GRAMMARS_INCLUDE_PREFIX} ${UHAL_UHAL_INCLUDE_PREFIX}  ${EMP_INCLUDE_PREFIX} ${EMP_LOG_INCLUDE_PREFIX})
link_directories( ${link_directories} ${UHAL_LOG_LIB_PREFIX} ${UHAL_GRAMMARS_LIB_PREFIX} ${UHAL_UHAL_LIB_PREFIX}  ${EMP_LIB_PREFIX} ${EMP_LOG_LIB_PREFIX})

file( GLOB executables ${PROJECT_SOURCE_DIR}/test/*.cxx ${PROJECT_SOURCE_DIR}/swamp/executables/*.cxx)

execute_process(
 COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-roc-regmap-hpp.py -i ${CMAKE_CURRENT_SOURCE_DIR}/reg_maps/si_rocv3b_regmap.yaml -o ${CMAKE_CURRENT_SOURCE_DIR}/swamp/include/generated_roc_register_map.hpp
 COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-roc-directaccess-regmap-hpp.py -i ${CMAKE_CURRENT_SOURCE_DIR}/reg_maps/si_roc_directaccess_regmap.yaml -o ${CMAKE_CURRENT_SOURCE_DIR}/swamp/include/generated_roc_direct_register_map.hpp
 COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-lpgbt-regmap-hpp.py -i ${CMAKE_CURRENT_SOURCE_DIR}/reg_maps/lpgbt_regmap.yaml -o ${CMAKE_CURRENT_SOURCE_DIR}/swamp/include/generated_lpgbt_register_map.hpp
 COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-econd-regmap-hpp.py -i ${CMAKE_CURRENT_SOURCE_DIR}/reg_maps/ECOND_I2C_params_regmap.json -o ${CMAKE_CURRENT_SOURCE_DIR}/swamp/include/generated_econd_register_map.hpp
 COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-econt-regmap-hpp.py -i ${CMAKE_CURRENT_SOURCE_DIR}/reg_maps/ECONT_I2C_params_regmap.json -o ${CMAKE_CURRENT_SOURCE_DIR}/swamp/include/generated_econt_register_map.hpp
 COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-vtrxp-regmap-hpp.py -i ${CMAKE_CURRENT_SOURCE_DIR}/reg_maps/vtrx_regmap.yaml -o ${CMAKE_CURRENT_SOURCE_DIR}/swamp/include/generated_vtrxp_register_map.hpp
)

add_library(objectlib OBJECT ${sources})
message( "-- Test executables to be installed")
foreach( exec ${executables} )
  message( "\t${exec}" )
  get_filename_component(ex ${exec} NAME_WE)
  # add_executable( ${ex} ${exec} ${headers} )
  add_executable( ${ex} ${exec} $<TARGET_OBJECTS:objectlib> ${headers} )
  target_link_libraries( ${ex} ${Boost_LIBRARIES} boost_program_options boost_regex boost_chrono boost_unit_test_framework cactus_uhal_log cactus_uhal_grammars cactus_uhal_uhal cactus_emp cactus_emp_logger )
  install( TARGETS ${ex} DESTINATION ${DESTINATION}/bin
    ARCHIVE DESTINATION ${DESTINATION}/lib
    LIBRARY DESTINATION ${DESTINATION}/lib
    )
endforeach()

add_library( ${PROJECT_NAME} SHARED ${sources} )
target_link_libraries( ${PROJECT_NAME} ${Boost_LIBRARIES} boost_program_options boost_regex boost_chrono boost_unit_test_framework cactus_uhal_log cactus_uhal_grammars cactus_uhal_uhal cactus_emp cactus_emp_logger )
target_include_directories( ${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include )
install( TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION ${DESTINATION}/lib
  LIBRARY DESTINATION ${DESTINATION}/lib
  )
install( FILES ${headers} DESTINATION ${DESTINATION}/include )

set( REPORT_FILE ${PROJECT_SOURCE_DIR}/TEST_REPORT.xml )
set( ROC_TEST_CMD ./build/roc )
add_custom_command(
  OUTPUT ${REPORT_FILE}
  COMMAND ${ROC_TEST_CMD} --logger=JUNIT,all,${REPORT_FILE} --color_output=true --detect_memory_leaks --show_progress=true
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  VERBATIM
  )

add_custom_target(TEST
  COMMAND echo "This is ALL target 'TEST', and it depends on ${REPORT_FILE}"
  # If the file exists, then commands related to that file won't be executed
  # DONOT let other target depends on the same OUTPUT as current target,
  #   or it may be bad when doing parallel make
  DEPENDS ${REPORT_FILE}
  VERBATIM
  )



string(TIMESTAMP TODAY "%Y.%m.%d.%H.%M.%S")
set(CPACK_PACKAGE_VERSION ${TODAY})
set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGE_NAME "swamp-cpp")
# set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_CONTACT "asteen")
set(CPACK_PACKAGE_VENDOR "hgcal")
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}.${CMAKE_SYSTEM_PROCESSOR}")

include(CPack)
