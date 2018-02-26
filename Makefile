# Variables
CC = g++ -std=c++11
FLAGS = -O3 -Wall
LIBS = `pkg-config --cflags --libs opencv` -lOpenCL

EXEC = gpu-filters
SOURCES = $(wildcard *.cpp) $(wildcard */*.cpp) # look for all .cpp files
OBJECTS = $(SOURCES:.cpp=.o) # list objects

# Main target
$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) -o $(EXEC)

# Object files
%.o: %.cpp
	$(CC) -c $(FLAGS) $< -o $@

clean:
	rm -f $(EXEC) $(OBJECTS)
