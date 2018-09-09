# Variables
CC = gcc
CXX = g++
CFLAGS = -std=c99
CXXFLAGS = -std=c++11
CPPFLAGS = $(EXFLAGS) -Wall -Wno-missing-braces -DCL_HPP_TARGET_OPENCL_VERSION=120 -DCL_HPP_MINIMUM_OPENCL_VERSION=120 -I . -I opencl/headers
CSOURCES = $(wildcard c/*.c)
CXXSOURCES = $(wildcard opencl/*.cpp ui/*.cpp)

OBJECTS = $(CSOURCES:.c=.o) $(CXXSOURCES:.cpp=.o)

ifeq ($(shell uname -s),Darwin)
LIBS = `pkg-config --cflags --libs opencv` \
	-lpthread
endif
ifeq ($(shell uname -s),Linux)
LIBS = -I /usr/include/opencv2 \
    -L /usr/lib \
    -lopencv_video \
    -lopencv_core -lopencv_highgui \
    -lpthread
endif


ifeq ($(shell uname -s),Darwin)
	LIBS += -framework OpenCL
endif
ifeq ($(shell uname -s),Linux)
	LIBS += -lOpenCL
endif

# Main
EXEC=gpu-filters

main: CPPFLAGS += -O3
main: $(EXEC)

debug: CPPFLAGS += -g
debug: $(EXEC)

$(EXEC): $(OBJECTS) main.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LIBS) -o $@

# Apps
APPS := inpainter linedetector

apps: CPPFLAGS += -O3
apps: $(APPS)

dapps: CPPFLAGS += -g
dapps: $(APPS)

$(APPS): % : $(OBJECTS) $(addprefix apps/, %.o)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LIBS) -o $@

clean:
	rm -f $(OBJECTS) $(EXEC) main.o $(APPS) $(addprefix apps/, $(APPS:=.o))

.PHONY: main debug apps clean

