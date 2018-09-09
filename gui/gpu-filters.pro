# Qt used components
QT += gui core widgets

# Static headers
PRECOMPILED_HEADER = static_headers.hpp

# include search paths
INCLUDEPATH += .. ../opencl/headers/

# Config
DEFINES += "CL_HPP_TARGET_OPENCL_VERSION=120"
DEFINES += "CL_HPP_MINIMUM_OPENCL_VERSION=120"
QMAKE_CXXFLAGS += -std=c++11 
CONFIG += object_with_source # put .o in same dir as source, so we can have sources with same names
CONFIG += precompile_header

# Extra libs 
LIBS += -lpthread `pkg-config --cflags --libs opencv` -lOpenCL

# Image processing sources
SOURCES += ../opencl/*.cpp
SOURCES += ../c/*.c
HEADERS += ../opencl/*.hpp ../c/*.h


# GUI sources
SOURCES += *.cpp
HEADERS += *.hpp
