CC = g++
FLAGS = -Wall -O3

all: clean libs.o

libs.o: libs.c libs.h
	$(CC) -c libs.c -o libs.o $(FLAGS)

clean:
	rm -f *.o
