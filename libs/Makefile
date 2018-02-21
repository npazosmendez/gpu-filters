CC = g++
FLAGS = -Wall -O3

all: clean libs.o clhelper.o

libs.o: libs.cpp libs.hpp clhelper.hpp
	$(CC) -c libs.cpp -o libs.o $(FLAGS)

clhelper.o: clhelper.cpp clhelper.hpp
	$(CC) -c clhelper.cpp -o clhelper.o $(FLAGS)

clean:
	rm -f *.o
