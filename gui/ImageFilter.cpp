#include <QtGui>
#include <iostream>
#include <algorithm>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencl/opencl-filters.hpp>
#include "ImageFilter.hpp"
#include "VideoStreamer.hpp"
#include "debug.h"

extern "C" {
    #include <c/c-filters.h>
}
    
using namespace std;
using namespace cv;

// Parent class

void ImageFilter::give_frame(Mat *frame){
    debug_print("Got frame of size %dx%d at %p, storing it at %p\n",
        frame->cols, frame->rows, frame, &_frame);
    _frame = *frame;
    if (cl){
        process_frame_CL(&_frame);
    }else{
        process_frame_C(&_frame);
    }
    debug_print("Finished processing frame at %p\n", &_frame);
    emit filtered_frame(&_frame);
    frame_processed = 1;
}

void ImageFilter::xxx(){
    debug_print("Filter xxx\n");
    return;
}
void ImageFilter::start(){
    return;
}

void ImageFilter::stop(){
    return;
}

void ImageFilter::toggle_CL(int val){
    if (val){
        cl = true;
    }else{
        cl = false;
    }
}


// Canny Filter

void CannyFilter::process_frame_CL(Mat *frame){
    CL_canny((char*)frame->ptr(), frame->size().width, frame->size().height, _higherThreshold, _lowerThreshold);
}

void CannyFilter::process_frame_C(Mat *frame){
    canny((char*)frame->ptr(), frame->size().width, frame->size().height, _higherThreshold, _lowerThreshold);
}

FilterControls* CannyFilter::controls(){
    return new CannyControls(this);
}


void CannyFilter::setHigherThreshold(int value){
    _higherThreshold = value;
}
void CannyFilter::setLowerThreshold(int value){
    _lowerThreshold = value;
}

// Kanade Filter
#define GRANULARITY 10
#define FACTOR 1

#define LINEAR(x,y) (y)*_width+(x)
#define forn(i,n) for(int i=0; i<(n); i++)
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

void KanadeFilter::process_frame_CL(Mat *frame){
    if (frame->size() != _last_frame.size()){
        _last_frame = *frame;
        return;
    }
    CL_kanade(frame->size().width, frame->size().height, (char*)_last_frame.ptr(), (char*)frame->ptr(), _flow, 3);

    _last_frame = *frame;
    overlay_flow(frame);
}

void KanadeFilter::process_frame_C(Mat *frame){
    if (frame->size() != _last_frame.size()){
        _last_frame = *frame;
        return;
    }
    kanade(frame->size().width, frame->size().height, (char*)_last_frame.ptr(), (char*)frame->ptr(), _flow, 3);

    _last_frame = *frame;
    overlay_flow(frame);
}

void KanadeFilter::overlay_flow(Mat* frame){
    int _width = frame->size().width;
    int _height = frame->size().height;
    
    forn(y, _height) forn(x, _width) if (y % GRANULARITY == 0 && x % GRANULARITY == 0) {
        vec displacement = _flow[LINEAR(x, y)];
        displacement.x *= FACTOR;
        displacement.y *= FACTOR;
        if (displacement.x != 0 || displacement.y != 0) {
            cv::line(
                *frame,
                Point(x, y),
                Point(x + displacement.x, y + displacement.y),
                Scalar( 0, 200, 0 ),
                1);
        }
    }
}

FilterControls* KanadeFilter::controls(){
    return new KanadeControls(this);
}

// Hough Filter

void HoughFilter::process_frame_CL(Mat *frame){
    CL_hough((char*)frame->ptr(), frame->size().width, frame->size().height, a_ammount, p_ammount, counter);
}

void HoughFilter::process_frame_C(Mat *frame){
    hough((char*)frame->ptr(), frame->size().width, frame->size().height,  a_ammount, p_ammount, counter);
}

FilterControls* HoughFilter::controls(){
    return new HoughControls(this);
}


HoughFilter::~HoughFilter() {
    delete counter;
}

HoughFilter::HoughFilter() :
    a_ammount(150), p_ammount(150), counter((char*)malloc(300*300*3)) {
        
}


void HoughFilter::setCannyHigherThreshold(int value){
    _canny_high = value;
}
void HoughFilter::setCannyLowerThreshold(int value){
    _canny_low = value;
}

// No Filter

FilterControls* NoFilter::controls(){
    return new NoControls(this);
}

void NoFilter::process_frame_C(Mat*){

}
void NoFilter::process_frame_CL(Mat*){

}