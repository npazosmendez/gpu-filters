CC = g++
FLAGS = -Wall -O3

all: clean hough.o

hough.o: hough.c hough.h
	$(CC) -c hough.c -o hough.o $(FLAGS)

clean:
	rm -f *.o
