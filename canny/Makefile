CC = g++
FLAGS = -Wall

all: clean canny.o

canny.o: canny.c canny.h
	$(CC) -c canny.c -o canny.o $(FLAGS)

clean:
	rm -f *.o
