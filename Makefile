CC = g++ -std=c++11
FLAGS = -O3 -Wall `pkg-config --cflags --libs opencv`

all: clean main

main:
	make -C canny
	make -C libs
	$(CC) main.cpp canny/canny.o libs/libs.o -o main $(FLAGS)

clean:
	rm -f main
