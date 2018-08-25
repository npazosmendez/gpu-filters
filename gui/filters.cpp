#include <QtGui>
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencl/opencl-filters.hpp>
#include "filters.hpp"

extern "C" {
    #include <c/c-filters.h>
}
    
using namespace std;
using namespace cv;

void CannyFilter::process_frame(Mat *cameraFrame){
    if (CL()){
        CL_canny((char*)cameraFrame->ptr(), cameraFrame->size().width, cameraFrame->size().height, 30, 60);
    }else{
        canny((char*)cameraFrame->ptr(), cameraFrame->size().width, cameraFrame->size().height, 30, 60);
    }
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