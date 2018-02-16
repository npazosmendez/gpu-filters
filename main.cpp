#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include "canny/canny.h"
#include "hough/hough.h"
#include <string>

using namespace cv;
using namespace std;

// Main variables
int width, height;
Mat cameraFrame;

// Canny variables
int hthreshold = 100;
int lthreshold = 40;

// UI misc
void on_mouse(int event, int x, int y, int flags, void* userdata);
void on_trackbar( int, void* ){}
void get_flags(int argc, char** argv);

// Flags
struct flags_t{
    bool file_input = false;
    string file_path;
};
flags_t flags;

int main(int argc, char** argv) {
    VideoCapture stream;

    get_flags(argc, argv);

    if (flags.file_input) {
        /* open video file */
        stream = VideoCapture(flags.file_path);
        width = stream.get(CV_CAP_PROP_FRAME_WIDTH);
        height = stream.get(CV_CAP_PROP_FRAME_HEIGHT);
    }else{
        /* open webcam */
        stream = VideoCapture(0); // video device number 0
        width = stream.get(CV_CAP_PROP_FRAME_WIDTH);
        height = stream.get(CV_CAP_PROP_FRAME_HEIGHT);
    }

    if (!stream.isOpened()) {
        cerr << "Failed to open stream." << endl;
        exit(1);
    }

    /* Main window */
    namedWindow("gpu-filters", 1);
    setMouseCallback("gpu-filters", on_mouse, NULL);

    /* Canny parameters trackbar */
    namedWindow("Canny Parameters", WINDOW_NORMAL);
    createTrackbar( "Higher Thresh", "Canny Parameters", &hthreshold, 400, on_trackbar);
    createTrackbar( "Lower Thresh", "Canny Parameters", &lthreshold, 400, on_trackbar);

    while (true) {
        if(!stream.read(cameraFrame)){ // get camera frame
            stream.set(CV_CAP_PROP_POS_MSEC,0); // reset if ended (for file streams)
            continue;
        }
        // resize(cameraFrame, cameraFrame, Size(640, 360), 0, 0, INTER_CUBIC); // uncomment to force 640x360 resolution

        char *ptr = (char*)cameraFrame.ptr();
        hough(ptr, width, height);
        // canny(ptr, width, height, hthreshold, lthreshold);
        // Canny(cameraFrame, cameraFrame, hthreshold, lthreshold);

        imshow("gpu-filters", cameraFrame); // show frame

        waitKey(5);
    }
    return 0;
}

void on_mouse(int event, int x, int y, int flags, void* userdata){
    /* handle mouse event */
    // will probably use for debugging
    unsigned char (*img)[width][3] = (unsigned char (*)[width][3])cameraFrame.ptr();
    switch (event) {
        case EVENT_LBUTTONDOWN:
        cout << "i: " << y << ", j:" << x << endl;
        img[y][x][0] = 0;
        img[y][x][1] = 0;
        img[y][x][2] = 255;
        imshow("gpu-filters", cameraFrame); // show frame
        waitKey(1000);
        break;
        case EVENT_RBUTTONDOWN:
        break;
        case EVENT_MBUTTONDOWN:
        break;
        case EVENT_MOUSEMOVE:
        break;
    }
}

void get_flags(int argc, char** argv){
    string param;
    for (int i = 0; i < argc; i++) {
        param = argv[i];
        if (param == "-i") {
            /* input file path */
            flags.file_input = true;
            if (i+1 == argc){
                cerr << "File path missing after '-i'." << endl;
                exit(1);
            }
            flags.file_path = argv[i+1];
        }else if(param == " "){
        }else if(param == " "){
        }else if(param == " "){

        }
    }
}
