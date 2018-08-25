
#include <QtGui>
#include <QLabel>
#include <QObject>
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
using namespace cv;
using namespace std;
extern "C" {
    #include <c/c-filters.h>
}
#include <opencl/opencl-filters.hpp>
#include "filters.hpp"
#include "camera.hpp"

#include <QAtomicInt>
#include <string>  

    
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
        CL_hough((char*)cameraFrame->ptr(), cameraFrame->size().width, cameraFrame->size().height, 150, 150, nullptr);
    }else{
        hough((char*)cameraFrame->ptr(), cameraFrame->size().width, cameraFrame->size().height, 150, 150, nullptr);
    }
    emit frame_ready(cameraFrame);

}