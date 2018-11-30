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
CPPFLAGS += -O3 -g

LIBS = `pkg-config --cflags --libs opencv` \
	-lpthread

ifeq ($(shell uname -s),Darwin)
	LIBS += -framework OpenCL
endif
ifeq ($(shell uname -s),Linux)
	LIBS += -lOpenCL
endif

all: timer inpainter realtime

timer: % : $(OBJECTS) $(addprefix timer_app/, %.o)
	$(CXX) $(CPPFLAGS) -g $(CXXFLAGS) $^ $(LIBS) -o $@

inpainter: % : $(OBJECTS) $(addprefix inpainter_app/, %.o)
	$(CXX) $(CPPFLAGS) -g $(CXXFLAGS) $^ $(LIBS) -o $@

realtime: % : $(OBJECTS)
	cd realtime_app; qmake && make && cp realtime ../realtime; cd ..

clean:
	rm -f $(OBJECTS) timer_app/timer.o timer inpainter_app/inpainter.o inpainter realtime
	cd realtime_app; make clean; cd ..

.PHONY: clean

