cmake_minimum_required (VERSION 3.1)
project("gpu-filters")

# set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Compilation
set(DEBUG_FLAGS -"D_GLIBCXX_DEBUG")
set(CMAKE_C_FLAGS "-std=c99")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wconversion -Wshadow -Wall -Wextra -Wno-missing-braces -DCL_HPP_TARGET_OPENCL_VERSION=200 -DCL_HPP_MINIMUM_OPENCL_VERSION=200")
# https://stackoverflow.com/questions/44942225/opencl-vector-types-cant-access-unioned-components-x-y-z-with-c11-enabled
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -U__STRICT_ANSI__")

# Libraries
find_package( OpenCV REQUIRED )
find_package( OpenCL REQUIRED )

# Sources and headers
file(GLOB FILTER_SOURCES c/*.c opencl/*.cpp)
# TODO: marking include/ as SYSTEM makes gcc ignore warnings from those headers. Make sure include/ only has headers we do not own
include_directories(SYSTEM include)
include_directories( . ${OpenCL_INCLUDE_DIRS})

# Apps targets
set(APPS filter tests flowcalculator inpainter linedetector timer)
foreach( app ${APPS})
    add_executable(${app} apps/${app}.cpp ${FILTER_SOURCES})
    target_link_libraries(${app} ${OpenCV_LIBS} ${OpenCL_LIBRARY} pthread)
endforeach( app )

# Tests targets
set(TESTS hough_test canny_test)
foreach( test ${TESTS})
    add_executable(${test} test/${test}.cpp test/utils.cpp ${FILTER_SOURCES})
    target_link_libraries(${test} ${OpenCV_LIBS} ${OpenCL_LIBRARY})
endforeach( test )