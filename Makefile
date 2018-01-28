CC = g++
FLAGS = -Wall `pkg-config --cflags --libs opencv`

all: clean main

main:
	make -C canny
	$(CC) main.cpp canny/canny.o -o main $(FLAGS)

clean:
	rm -f main
