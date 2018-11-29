# Variables
CC = gcc
CXX = g++
CFLAGS = -std=c99
CXXFLAGS = -std=c++11
CPPFLAGS = $(EXFLAGS) -Wall -Wshadow -Wno-missing-braces -DCL_HPP_TARGET_OPENCL_VERSION=120 -DCL_HPP_MINIMUM_OPENCL_VERSION=120 -I . -I include/
CSOURCES = $(wildcard c/*.c)
CXXSOURCES = $(wildcard opencl/*.cpp)

OBJECTS = $(CSOURCES:.c=.o) $(CXXSOURCES:.cpp=.o)

# https://stackoverflow.com/questions/44942225/opencl-vector-types-cant-access-unioned-components-x-y-z-with-c11-enabled
CPPFLAGS += -U__STRICT_ANSI__

LIBS = `pkg-config --cflags --libs opencv` \
	-lpthread

ifeq ($(shell uname -s),Darwin)
	LIBS += -framework OpenCL
endif
ifeq ($(shell uname -s),Linux)
	LIBS += -lOpenCL
endif

# Apps
APPS := inpainter linedetector flowcalculator tests timer filter

TESTS := canny_test hough_test

apps: CPPFLAGS += -O3 -g
apps: $(APPS)

dapps: CPPFLAGS += -g
dapps: $(APPS)

$(APPS): % : $(OBJECTS) $(addprefix apps/, %.o)
	$(CXX) $(CPPFLAGS) -g $(CXXFLAGS) $^ $(LIBS) -o $@

$(TESTS): % : $(OBJECTS)  test/utils.o $(addprefix test/, %.o)
	$(CXX) $(CPPFLAGS) -g $(CXXFLAGS) $^ $(LIBS) -o $@

clean:
	rm -f $(OBJECTS) $(APPS) $(TESTS) $(addprefix apps/, $(APPS:=.o)) $(addprefix tests/, $(TESTS:=.o))

.PHONY: apps clean

