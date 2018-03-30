# Variables
CC = gcc
CXX = g++
CFLAGS = -std=c99
CXXFLAGS = -std=c++11
CPPFLAGS = -g -O3 -Wall -I opencl/headers
CSOURCES = $(wildcard */*.c)
CXXSOURCES = $(wildcard *.cpp) $(wildcard */*.cpp)

EXEC = gpu-filters
OBJECTS = $(CSOURCES:.c=.o) $(CXXSOURCES:.cpp=.o)
LIBS = `pkg-config --cflags --libs opencv` 
ifeq ($(shell uname -s),Darwin)
	LIBS += -framework OpenCL
endif
ifeq ($(shell uname -s),Linux)
	LIBS += -lOpenCL
endif


# Main target
$(EXEC): $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(OBJECTS) $(LIBS) -o $(EXEC)

clean:
	rm -f $(EXEC) $(OBJECTS)
