SOURCES = main.cpp ../opencl/hough.cpp ../opencl/canny.cpp ../opencl/clutils.cpp
SOURCES += ../c/*.c
SOURCES += filters.cpp camera.cpp
SOURCES += hitcounter.cpp window.cpp
HEADERS += camera.hpp filters.hpp hitcounter.hpp window.hpp
INCLUDEPATH = .. ../opencl/headers/
QMAKE_CXXFLAGS = -std=c++11 
CONFIG += object_with_source # put .o in same dir as source, so we can have sources with same names
LIBS = -lpthread `pkg-config --cflags --libs opencv` -lOpenCL
DEFINES += "CL_HPP_TARGET_OPENCL_VERSION=120"
DEFINES += "CL_HPP_MINIMUM_OPENCL_VERSION=120"
QT += gui core widgets