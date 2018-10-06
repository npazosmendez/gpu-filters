//
// Created by Daro on 23/09/2018.
//

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
extern "C" {
#include "c/c-filters.h"
}
#include "opencl/opencl-filters.hpp"

using namespace cv;
using namespace std;

#define GRANULARITY 5
#define FACTOR 1

#define LINEAR(x,y) (y)*width+(x)
#define forn(i,n) for(int i=0; i<(n); i++)

bool quit = false;

int width, height;
Mat cameraFrame;
Mat drawnFrame;

int main(int argc, char** argv) {

    // Initialize stuff

    VideoCapture stream = VideoCapture(0);
    width = stream.get(CV_CAP_PROP_FRAME_WIDTH);
    height = stream.get(CV_CAP_PROP_FRAME_HEIGHT);

    if (!stream.isOpened()) {
        cerr << "Failed to open stream." << endl;
        exit(1);
    }

    namedWindow("video", WINDOW_NORMAL);
    resizeWindow("video", 400,400);
    moveWindow("video",0,0);

    // Init flow buffer

    vec * flow = (vec*) malloc(sizeof(vec) * width * height);

    // Show video and apply filter

    char * current_ptr = nullptr;
    char * old_ptr = (char*) malloc(3 * sizeof(char) * width * height);

    while (!stream.read(cameraFrame)) {
        stream.set(CV_CAP_PROP_POS_MSEC, 0);
    }

    current_ptr = (char*) cameraFrame.ptr();

    cv::waitKey(5);

    while (true) {

        memcpy(old_ptr, current_ptr, 3 * sizeof(char) * width * height);

        if(!stream.read(cameraFrame)){ // get camera frame
            stream.set(CV_CAP_PROP_POS_MSEC,0); // reset if ended (for file streams)
            continue;
        }

        current_ptr = (char*)cameraFrame.ptr();

        kanade(width, height, old_ptr, current_ptr, flow);

        drawnFrame = cameraFrame.clone();
        forn(y, height) forn(x, width) if (y % GRANULARITY == 0 && x % GRANULARITY == 0) {
            vec displacement = flow[LINEAR(x, y)];
            displacement.x *= FACTOR;
            displacement.y *= FACTOR;
            if (displacement.x != 0 || displacement.y != 0) {
                arrowedLine(drawnFrame, Point(x, y), Point(x + displacement.x, y + displacement.y), Scalar( 0, 200, 0 ), 1);
            }
        }

        imshow("gpu-filters", drawnFrame); // show frame

        if ((char)cv::waitKey(5) == 'q') break;
    }



}