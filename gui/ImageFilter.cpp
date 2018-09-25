#include <QtGui>
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencl/opencl-filters.hpp>
#include "ImageFilter.hpp"
#include "VideoStreamer.hpp"

extern "C" {
    #include <c/c-filters.h>
}
    
using namespace std;
using namespace cv;

// Parent class

void ImageFilter::give_frame(Mat *frame){
    _frame = *frame;
    if (cl){
        process_frame_CL(&_frame);
    }else{
        process_frame_C(&_frame);
    }
    emit filtered_frame(&_frame);
    frame_processed = 1;
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
    CL_canny((char*)frame->ptr(), frame->size().width, frame->size().height, _lowerThreshold, _higherThreshold);
}

void CannyFilter::process_frame_C(Mat *frame){
    canny((char*)frame->ptr(), frame->size().width, frame->size().height, _lowerThreshold, _higherThreshold);
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

// Hough Filter

void HoughFilter::process_frame_CL(Mat *frame){
    CL_hough((char*)frame->ptr(), frame->size().width, frame->size().height, 150, 150, counter);
}

void HoughFilter::process_frame_C(Mat *frame){
    hough((char*)frame->ptr(), frame->size().width, frame->size().height, 150, 150, counter);
}

FilterControls* HoughFilter::controls(){
    return new HoughControls(this);
}


HoughFilter::~HoughFilter() {
    delete counter;
}

HoughFilter::HoughFilter() :
    a_ammount(150), p_ammount(150), counter((char*)malloc(150*150*3)) {
        
}


// No Filter

FilterControls* NoFilter::controls(){
    return new NoControls(this);
}

void NoFilter::process_frame_C(Mat *frame){

}
void NoFilter::process_frame_CL(Mat *frame){

}