CC = g++
FLAGS = -Wall -O3

all: clean hough.o

hough.o: hough.cpp hough.hpp
	$(CC) -c hough.cpp -o hough.o $(FLAGS)

clean:
	rm -f *.o
