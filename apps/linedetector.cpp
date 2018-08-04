//
// Created by nicolas on 08/07/18.
//

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <iomanip>
#include <chrono>
extern "C" {
#include "c/c-filters.h"
}
#include "opencl/opencl-filters.hpp"

using namespace cv;
using namespace std;
#include <pthread.h>
#include <condition_variable>
#include <mutex>

// Main variables
int width, height;
Mat frame1;
VideoCapture stream;

bool frame_requested = false;
bool free_zone = false;
condition_variable condition_var;
mutex mtx;


void* flush_stream(void *arg){
    while(true){
        if (!frame_requested){
            free_zone = false;
            stream.grab();
        }else{
            unique_lock<mutex> lck(mtx);
            condition_var.notify_all();
        }
    }
    pthread_exit(NULL);
}


int main(int argc, char** argv) {
    bool image = false;
    // Read Images
    if (argc > 1) {
        image = true;
        frame1 = imread(argv[1], CV_LOAD_IMAGE_COLOR);
        width = frame1.size().width;
        height = frame1.size().height;
    }else{
        /* open webcam */
        stream = VideoCapture(0); // video device number 0
        width = stream.get(CV_CAP_PROP_FRAME_WIDTH);
        height = stream.get(CV_CAP_PROP_FRAME_HEIGHT);

        if (!stream.isOpened()) {
            cerr << "Failed to open stream." << endl;
            exit(1);
        }
    }


    /* Main window */
    namedWindow("counter", WINDOW_NORMAL);
    namedWindow("video", WINDOW_NORMAL);
    resizeWindow("counter", 400,400);
    resizeWindow("video", 400,400);
    moveWindow("video",0,0);
    moveWindow("counter",0,500);

    int a_ammount, p_ammount;
    a_ammount = 150;
    p_ammount = 150;
    Mat counter(Size(a_ammount, p_ammount), CV_8UC3);
    pthread_t thread;
    int ret = pthread_create(&thread, NULL, &flush_stream, NULL);
    if (ret != 0){
        exit(1);
    }
    /*
    */
    while (true) {
        frame_requested = true;
        unique_lock<std::mutex> lck(mtx);
        condition_var.wait(lck);
        stream.read(frame1);
        frame_requested = false;
 
        char *ptr = (char*)frame1.ptr();
        char *ptrc = (char*)counter.ptr();
        hough(ptr, width, height, a_ammount, p_ammount,ptrc);
        imshow("counter", counter); // show frame
        imshow("video", frame1); // show frame
        int time = 5;
         if (image){
            time = 100000;
        }
        if ((char)cv::waitKey(time) == 'q') break;
    }

    pthread_join(thread, NULL);
    return 0;
}