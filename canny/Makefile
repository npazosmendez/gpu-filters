CC = g++
FLAGS = -Wall -O3


all: clean canny.o

canny.o: canny.cpp canny.hpp
	$(CC) -c canny.cpp -o canny.o $(FLAGS)

clean:
	rm -f *.o
