#include "VideoStreamer.hpp"
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <QAtomicInt>
#include <chrono>

using namespace cv;
using namespace std;
extern uint qGlobalPostedEventsCount(); // from qapplication.cpp

QAtomicInt frame_processed = 1;

VideoFileStreamer::VideoFileStreamer(string filename){
	/* open webcam */
	_filename = filename;
    stream = VideoCapture(filename);
    width = stream.get(CV_CAP_PROP_FRAME_WIDTH);
    height = stream.get(CV_CAP_PROP_FRAME_HEIGHT);
	if (!stream.isOpened()) {
	    cerr << "Failed to open stream." << endl;
	    abort();
	}
	_fps = stream.get(CV_CAP_PROP_FPS);
}

void VideoFileStreamer::run(){
	_stop = false;

	frame_processed = 1;
	long last_frame_ms = 0;
	float frame_ms_period = 1000.0 / _fps;

	while(!_stop){

	    if ( milliseconds_since_epoch() - last_frame_ms < frame_ms_period) continue;

		bool ready = frame_processed.testAndSetAcquire(1, 0);
	    if (ready){
		    if(not stream.read(_buffer[index])){
		    	frame_processed.testAndSetAcquire(0, 1);
		    	stream.open(_filename);
		    	continue;
		    }
		    last_frame_ms = milliseconds_since_epoch();
			emit emit_frame(&_buffer[index]);
			index = (index+1) % 4;
	    }else{
	    	continue;
	    }
	}
}

void VideoFileStreamer::stop(){
	_stop = true;
}

long VideoFileStreamer::milliseconds_since_epoch(){
	auto now = chrono::high_resolution_clock::now();
    return chrono::time_point_cast<chrono::milliseconds>(now).time_since_epoch().count();
}


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
