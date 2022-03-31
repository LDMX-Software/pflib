cmake_minimum_required(VERSION 3.12)

project(PolarfireBridge LANGUAGES CXX)

# Check cmake version
include(InstallRequiredSystemLibraries)

# C/C++
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wno-deprecated")

if (DEFINED ENV{ROGUE_DIR})
   set(Rogue_DIR $ENV{ROGUE_DIR}/lib)
else()
   set(Rogue_DIR /usr/local/lib)
endif()
find_package(Rogue REQUIRED)

add_executable(polarfire_wb_server src/PolarfireWB.cxx src/polarfire_wb_server.cxx)
target_include_directories(polarfire_wb_server PRIVATE ${ROGUE_INCLUDE_DIRS})
target_link_libraries(polarfire_wb_server PUBLIC pthread ${ROGUE_LIBRARIES})

add_executable(dma-read src/DMAReader.cxx src/dma_read.cpp)
target_include_directories(dma-read PRIVATE ${ROGUE_INCLUDE_DIRS})
target_link_libraries(dma-read PUBLIC pthread ${ROGUE_LIBRARIES})
