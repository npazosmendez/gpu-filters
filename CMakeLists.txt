cmake_minimum_required (VERSION 3.1)
project("gpu-filters")

# -- Config -- #
set(DEBUG_FLAGS -"D_GLIBCXX_DEBUG")
set(CMAKE_C_FLAGS "-std=c99")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wconversion -Wshadow -Wall -Wextra -Wno-missing-braces -DCL_HPP_TARGET_OPENCL_VERSION=200 -DCL_HPP_MINIMUM_OPENCL_VERSION=200")
# https://stackoverflow.com/questions/44942225/opencl-vector-types-cant-access-unioned-components-x-y-z-with-c11-enabled
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -U__STRICT_ANSI__")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# set(CMAKE_VERBOSE_MAKEFILE on)

# -- Libraries -- #
find_package( OpenCV REQUIRED )
find_package( OpenCL REQUIRED )

# -- Sources -- #
file(GLOB FILTER_SOURCES src/c/*.c src/opencl/*.cpp)
include_directories(SYSTEM src/libs)
include_directories( src ${OpenCL_INCLUDE_DIRS} generated)

# -- Targets -- #

# Filters
set(GENERATED_KERNELS_PATH ${CMAKE_BINARY_DIR}/generated/kernels.h)
ADD_CUSTOM_COMMAND(
    OUTPUT ${GENERATED_KERNELS_PATH}
    COMMENT "Generating kernels."
    DEPENDS generate_kernels.py src/opencl/*.cl
    COMMAND python3 generate_kernels.py ${GENERATED_KERNELS_PATH}
)
add_library(filters STATIC ${FILTER_SOURCES} ${GENERATED_KERNELS_PATH})
target_link_libraries(filters ${OpenCL_LIBRARY})

# Apps
set(APPS filter flowcalculator inpainter linedetector timer)
foreach( app ${APPS})
    add_executable(${app} apps/${app}.cpp)
    target_link_libraries(${app} ${OpenCV_LIBS} pthread filters)
endforeach( app )

# Tests
set(TESTS hough_test canny_test misc_test)
foreach( test ${TESTS})
    add_executable(${test} test/${test}.cpp test/utils.cpp)
    target_link_libraries(${test} ${OpenCV_LIBS} filters)
endforeach( test )