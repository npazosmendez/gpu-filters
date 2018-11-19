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

#define LINEAR(x,y) (y)*width+(x)
#define forn(i,n) for(int i=0; i<(n); i++)


#define WIDTH 640
#define HEIGHT 480

#define LEVELS 1
#define GRANULARITY 2
#define FACTOR 1
#define MIN_LENGTH 0


bool quit = false;

int width, height;
Mat cameraFrame;
Mat drawnFrame;

int main(int argc, char** argv) {

    // Params

    void (*picked_kanade)(int, int, char*, char*, vecf*, int) = kanade;

    for (int i = 1; i < argc; i++) {
        string param = argv[i];
        if (param == "-cl") {
            /* implementation selection */
            cout << "Using OpenCL implementation..." << endl;
            picked_kanade = CL_kanade;
        } else if (param == "-d") {
            /* device selection */
            if (i+1 == argc) {
                cerr << "Device name missing after '-d'." << endl;
                exit(1);
            }
            int device_number;
            try {
                device_number = stoi(argv[i+1]);
            } catch (int e) {
                cerr << "Couldn't convert '-d' parameter to number" << endl;
                return -1;
            }
            selectDevice(device_number);
        } else if (param == "-h") {
            cout << "Usage:\n";
            cout << "  flowcalculator [flags]\n";
            cout << "Flags\n";
            cout << "  -cl: Uses OpenCL implementation instead of the C implementation\n";
            cout << "  -d <number>: Picks device to use for the computation\n";
            cout << "\n";

            return 0;
        }
    }

    // Initialize stuff

    VideoCapture stream = VideoCapture(0);
    stream.set(CV_CAP_PROP_FRAME_WIDTH, WIDTH);
    stream.set(CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);

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

    vecf * flow = (vecf*) malloc(sizeof(vecf) * width * height);

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

        picked_kanade(width, height, old_ptr, current_ptr, flow, LEVELS);

        drawnFrame = cameraFrame.clone();
        forn(y, height) forn(x, width) if (y % GRANULARITY == 0 && x % GRANULARITY == 0) {
            vecf displacement = flow[LINEAR(x, y)];
            displacement.x *= FACTOR;
            displacement.y *= FACTOR;
            if (abs(int(displacement.x)) > MIN_LENGTH || abs(int(displacement.y)) > MIN_LENGTH) {
                line(
                        drawnFrame,                                    // mat to draw into
                        Point(x, y),                                   // origin position
                        Point(x + int(displacement.x), y + int(displacement.y)), // arrow tip position
                        Scalar( 0, 200, 0 ),                           // color
                        1                                              // thickness
                );
            }
            /*
            // For corner detection
            if (displacement.x == 1) {
                circle(
                        drawnFrame,                                    // mat to draw into
                        Point(x, y),                                   // origin position
                        1,                                             // radius
                        Scalar( 0, 200, 200 )                          // color
                );
            }
             */
        }

        imshow("gpu-filters", drawnFrame); // show frame

        if ((char)cv::waitKey(5) == 'q') break;
    }



}