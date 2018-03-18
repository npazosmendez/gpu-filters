# Variables
CC = gcc
CXX = g++
CFLAGS = -std=c99
CXXFLAGS = -std=c++11
CPPFLAGS = -O3 -Wall $(CPPLIBS)
CSOURCES = $(wildcard */*.c)
CXXSOURCES = $(wildcard *.cpp) $(wildcard */*.cpp)

EXEC = gpu-filters
OBJECTS = $(CSOURCES:.c=.o) $(CXXSOURCES:.cpp=.o)
LIBS = `pkg-config --cflags --libs opencv` -framework OpenCL

# Main target
$(EXEC): $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(OBJECTS) $(LIBS) -o $(EXEC)

clean:
	rm -f $(EXEC) $(OBJECTS)
