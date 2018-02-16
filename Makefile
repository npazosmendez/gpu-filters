CC = g++ -std=c++11
FLAGS = -O3 -Wall `pkg-config --cflags --libs opencv`

all: clean main

main:
	make -C canny
	make -C hough
	make -C libs
	$(CC) main.cpp canny/canny.o hough/hough.o libs/libs.o -o main $(FLAGS)

clean:
	rm -f main
