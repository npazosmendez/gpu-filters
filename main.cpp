#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <string>
extern "C" {
    #include "c/c-filters.h"
}
#include "opencl/opencl-filters.hpp"

using namespace cv;
using namespace std;

// Main variables
int width, height;
Mat cameraFrame;

// Canny variables
int hthreshold = 50;
int lthreshold = 25;

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

    /* Init OpenCL */
    initCL();

    while (true) {
        if(!stream.read(cameraFrame)){ // get camera frame
            stream.set(CV_CAP_PROP_POS_MSEC,0); // reset if ended (for file streams)
            continue;
        }
        // resize(cameraFrame, cameraFrame, Size(640, 360), 0, 0, INTER_CUBIC); // uncomment to force 640x360 resolution

        char *ptr = (char*)cameraFrame.ptr();
        // hough(ptr, width, height);
        // CL_canny(ptr, width, height, hthreshold, lthreshold);
        canny(ptr,width,height,hthreshold,lthreshold);

        imshow("gpu-filters", cameraFrame); // show frame

        waitKey(5);
    }
    return 0;
}

void on_mouse(int event, int x, int y, int flags, void* userdata){
    // handle mouse event
    // will probably use for debugging
    unsigned char * img = (unsigned char*)cameraFrame.ptr();
    switch (event) {
        case EVENT_LBUTTONDOWN:
        cout << "i: " << y << ", j:" << x << endl;
        // Puede fallar
        img[3 * width * y + 3 * x + 0] = 0;
        img[3 * width * y + 3 * x + 1] = 0;
        img[3 * width * y + 3 * x + 2] = 0;
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
