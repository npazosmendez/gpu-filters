#include "camera.hpp"
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <QAtomicInt>

using namespace cv;
using namespace std;
extern uint qGlobalPostedEventsCount(); // from qapplication.cpp


Camera::Camera(){
	/* open webcam */
	stream = VideoCapture(0); // video device number 0
	width = stream.get(CV_CAP_PROP_FRAME_WIDTH);
	height = stream.get(CV_CAP_PROP_FRAME_HEIGHT);
	if (!stream.isOpened()) {
	    cerr << "Failed to open stream." << endl;
	    int a = 1/0;
	}
    stream.read(_buffer[0]);
    stream.read(_buffer[1]);
    stream.read(_buffer[2]);
    stream.read(_buffer[3]);
}

void Camera::run(){
	frame_processed = 1;
	_stop = false;
	while(!_stop){
	    stream.read(_buffer[index]);
	    if (frame_processed){
		    frame_processed = 0;
			emit emit_frame(&_buffer[index]);
			index = (index+1) % 4;
	    }
	}
}

void Camera::stop(){
	_stop = true;
}
