cmake_minimum_required(VERSION 3.12)

project(dmaRead LANGUAGES CXX)

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

add_executable(dma-read dma_read.cpp)
target_include_directories(dma-read PRIVATE ${ROGUE_INCLUDE_DIRS})
target_link_libraries(dma-read PUBLIC pthread ${ROGUE_LIBRARIES})
