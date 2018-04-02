# Variables
CC = gcc
CXX = g++
CFLAGS = -std=c99
CXXFLAGS = -std=c++11
CPPFLAGS = -Wall -I . -I opencl/headers
CSOURCES = $(wildcard c/*.c)
CXXSOURCES = $(wildcard opencl/*.cpp)

OBJECTS = $(CSOURCES:.c=.o) $(CXXSOURCES:.cpp=.o)
LIBS = `pkg-config --cflags --libs opencv` 
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
APPS := picture

apps: CPPFLAGS += -O3
apps: $(APPS)

dapps: CPPFLAGS += -g
dapps: $(APPS)

$(APPS): % : $(OBJECTS) $(addprefix apps/, %.o)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LIBS) -o $@

clean:
	rm -f $(OBJECTS) $(EXEC) main.o $(APPS) $(addprefix apps/, $(APPS:=.o))

.PHONY: main debug apps clean

