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

void CannyFilter::process_frame(Mat *cameraFrame){
    if (CL()){
        CL_canny((char*)cameraFrame->ptr(), cameraFrame->size().width, cameraFrame->size().height, _lowerThreshold, _higherThreshold);
    }else{
        canny((char*)cameraFrame->ptr(), cameraFrame->size().width, cameraFrame->size().height, _lowerThreshold, _higherThreshold);
    }
    emit frame_ready(cameraFrame);

}

HoughFilter::~HoughFilter() {
    delete counter;
    delete _controls;
}
        
HoughFilter::HoughFilter() : 
    a_ammount(150), p_ammount(150), counter((char*)malloc(150*150*3)) {
    _controls = new HoughControls(this);
        
}
CannyFilter::CannyFilter() { 
    _controls = new CannyControls(this);

}
CannyFilter::~CannyFilter() { 
    delete _controls;

}

void NoFilter::process_frame(Mat *cameraFrame){
    emit frame_ready(cameraFrame);

}
void HoughFilter::process_frame(Mat *cameraFrame){
    if (CL()){
        CL_hough((char*)cameraFrame->ptr(), cameraFrame->size().width, cameraFrame->size().height, 150, 150, counter);
    }else{
        hough((char*)cameraFrame->ptr(), cameraFrame->size().width, cameraFrame->size().height, 150, 150, counter);
    }
    emit frame_ready(cameraFrame);

}

FilterControls& CannyFilter::controls(){
    return *_controls;
}
void CannyFilter::setHigherThreshold(int value){
    _higherThreshold = value;
}
void CannyFilter::setLowerThreshold(int value){
    _lowerThreshold = value;
}

FilterControls& HoughFilter::controls(){
    return *_controls;
}

FilterControls& NoFilter::controls(){
    return *_controls;
}
