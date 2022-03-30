cmake_minimum_required(VERSION 3.12)

project(PolarfireWBBride LANGUAGES CXX)

# Add support for building in conda environment
if (DEFINED ENV{CONDA_PREFIX})
   set(CMAKE_PREFIX_PATH "$ENV{CONDA_PREFIX}")
   link_directories($ENV{CONDA_PREFIX}/lib)
endif()

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

add_executable(polarfire_wb_server ${PROJECT_SOURCE_DIR}/src/PolarfireWB.cxx ${PROJECT_SOURCE_DIR}/src/polarfire_wb_server.cxx)
target_include_directories(polarfire_wb_server PRIVATE ${ROGUE_INCLUDE_DIRECTORIES} include)
target_link_libraries(polarfire_wb_server PUBLIC pthread ${ROGUE_LIBRARIES})
