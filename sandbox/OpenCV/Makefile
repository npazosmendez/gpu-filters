CC = g++
FLAGS = -Wall `pkg-config --cflags --libs opencv`

all: opencv

opencv: opencv.cpp
	$(CC) opencv.cpp -o opencv $(FLAGS)
