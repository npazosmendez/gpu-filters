#include <QtGui>
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencl/opencl-filters.hpp>
#include "ImageFilter.hpp"

extern "C" {
    #include <c/c-filters.h>
}
    
using namespace std;
using namespace cv;

// Parent class

void ImageFilter::process_frame(Mat *cameraFrame){
    if (cl){
        process_frame_CL(cameraFrame);
    }else{
        process_frame_C(cameraFrame);
    }
    emit frame_ready(cameraFrame);
}

void ImageFilter::toggleCL(int val){
    if (val){
        cl = true;
    }else{
        cl = false;
    }
}


// Canny Filter

void CannyFilter::process_frame_CL(Mat *cameraFrame){
    CL_canny((char*)cameraFrame->ptr(), cameraFrame->size().width, cameraFrame->size().height, _lowerThreshold, _higherThreshold);
}

void CannyFilter::process_frame_C(Mat *cameraFrame){
    canny((char*)cameraFrame->ptr(), cameraFrame->size().width, cameraFrame->size().height, _lowerThreshold, _higherThreshold);
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

void HoughFilter::process_frame_CL(Mat *cameraFrame){
    CL_hough((char*)cameraFrame->ptr(), cameraFrame->size().width, cameraFrame->size().height, 150, 150, counter);
}

void HoughFilter::process_frame_C(Mat *cameraFrame){
    hough((char*)cameraFrame->ptr(), cameraFrame->size().width, cameraFrame->size().height, 150, 150, counter);
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
