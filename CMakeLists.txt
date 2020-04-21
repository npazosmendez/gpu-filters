cmake_minimum_required (VERSION 3.1)
project("gpu-filters")

# Compilation
set(DEBUG_FLAGS -"D_GLIBCXX_DEBUG")
set(CMAKE_C_FLAGS "-std=c99")
# TODO: compile c++11, need to update cl headers for that
set(CMAKE_CXX_FLAGS "-Wconversion -Wshadow -Wall -Wextra -Wno-missing-braces -DCL_HPP_TARGET_OPENCL_VERSION=200 -DCL_HPP_MINIMUM_OPENCL_VERSION=200")

# Libraries
find_package( OpenCV REQUIRED )
find_package( OpenCL REQUIRED )

set(CMAKE_VERBOSE_MAKEFILE on)

# Sources and headers
file(GLOB FILTER_SOURCES c/*.c opencl/*.cpp)
# TODO: marking include/ as SYSTEM makes gcc ignore warnings from those headers. Make sure include/ only has headers we do not own
include_directories(SYSTEM include)
include_directories( . ${OpenCL_INCLUDE_DIRS})

# Targets
set(APPS
    filter
    tests
    flowcalculator
    inpainter
    linedetector
    timer
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
foreach( app ${APPS})
    add_executable(${app} apps/${app}.cpp ${FILTER_SOURCES})
    target_link_libraries(${app} ${OpenCV_LIBS} ${OpenCL_LIBRARY} pthread)
endforeach( app )
