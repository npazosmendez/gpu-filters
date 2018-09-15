#include "camera.hpp"
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <QAtomicInt>

using namespace cv;
using namespace std;

Camera::Camera(){
	/* open webcam */
	stream = VideoCapture(0); // video device number 0
	width = stream.get(CV_CAP_PROP_FRAME_WIDTH);
	height = stream.get(CV_CAP_PROP_FRAME_HEIGHT);
	if (!stream.isOpened()) {
	    cerr << "Failed to open stream." << endl;
	    exit(1);
	}
    stream.read(_buffer[0]);
    stream.read(_buffer[1]);
    stream.read(_buffer[2]);
    stream.read(_buffer[3]);
}

void Camera::run(){
	while(1){
	    stream.read(_buffer[index]);
	}
}

void Camera::request_frame(){
    stream.read(_buffer[index]);
	emit emit_frame(&_buffer[index]);
	index = (index+1) % 4;
}