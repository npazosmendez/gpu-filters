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
Mat frame;

// Canny variables
int uthreshold = 50;
int lthreshold = 20;

// UI misc
void on_mouse(int event, int x, int y, int flags, void* userdata);
void on_trackbar( int, void* ){}
void get_flags(int argc, char** argv);

// Flags
enum filter_t   {C_CANNY,C_HOUGH,C_INPAINTING,C_LUCASKANADE,
    OCL_CANNY,OCL_HOUGH,OCL_INPAINTING,OCL_LUCASKANADE};

struct flags_t{
    bool video_input = false;
    bool image_input = false;
    string video_path;
    string image_path;
    filter_t filter = C_CANNY;
};
flags_t flags;


int main(int argc, char** argv) {
    VideoCapture stream;

    get_flags(argc, argv);

    if (flags.video_input) {
        /* open video file */
        stream = VideoCapture(flags.video_path);
        width = stream.get(CV_CAP_PROP_FRAME_WIDTH);
        height = stream.get(CV_CAP_PROP_FRAME_HEIGHT);
    }else if(flags.image_input){
        frame = imread(flags.image_path, 1);
        height = frame.rows;
        width = frame.cols;
    }else{
        /* open webcam */
        stream = VideoCapture(0); // video device number 0
        width = stream.get(CV_CAP_PROP_FRAME_WIDTH);
        height = stream.get(CV_CAP_PROP_FRAME_HEIGHT);
    }

    if (flags.image_input) {
        char *ptr = (char*)frame.ptr();
        switch (flags.filter) {
            case C_CANNY:
            canny(ptr,width,height,uthreshold,lthreshold);
            break;
            case OCL_CANNY:
            CL_canny(ptr, width, height, uthreshold, lthreshold);
            break;
            case C_HOUGH:
            hough(ptr, width, height);
            break;
            default:
            break;
        }
        imwrite("output.bmp",frame);
        return 0;
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
    createTrackbar( "Higher Thresh", "Canny Parameters", &uthreshold, 100, on_trackbar);
    createTrackbar( "Lower Thresh", "Canny Parameters", &lthreshold, 100, on_trackbar);

    /* Init OpenCL */
    initCL();

    while (true) {
        if(!stream.read(frame)){ // get camera frame
            stream.set(CV_CAP_PROP_POS_MSEC,0); // reset if ended (for file streams)
            continue;
        }

        char *ptr = (char*)frame.ptr();

        switch (flags.filter) {
            case C_CANNY:
            canny(ptr,width,height,uthreshold,lthreshold);
            break;
            case OCL_CANNY:
            CL_canny(ptr, width, height, uthreshold, lthreshold);
            break;
            case C_HOUGH:
            hough(ptr, width, height);
            break;
            default:
            break;
        }

        imshow("gpu-filters", frame); // show frame

        waitKey(5);
    }
    return 0;
}

void on_mouse(int event, int x, int y, int flags, void* userdata){
    // handle mouse event
    // will probably use for debugging
    unsigned char * img = (unsigned char*)frame.ptr();
    switch (event) {
        case EVENT_LBUTTONDOWN:
        cout << "i: " << y << ", j:" << x << endl;
        // Puede fallar
        img[3 * width * y + 3 * x + 0] = 0;
        img[3 * width * y + 3 * x + 1] = 0;
        img[3 * width * y + 3 * x + 2] = 0;
        imshow("gpu-filters", frame); // show frame
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
    // TODO: add help()
    string param;
    for (int i = 0; i < argc; i++) {
        param = argv[i];
        if (param == "-iv") {
            /* input video file path */
            flags.video_input = true;
            if (i+1 == argc){
                cerr << "File path missing after '-iv'." << endl;
                exit(1);
            }
            flags.video_path = argv[i+1];
        }else if(param == "-f"){
            /* filter selection */
            if (i+1 == argc){
                cerr << "Filter name missing after '-f'." << endl;
                exit(1);
            }
            string filter = argv[i+1];
            if (filter == "c-canny") {
                flags.filter = C_CANNY;
            }else if(filter == "c-hough"){
                flags.filter = C_HOUGH;
            }else if(filter == "c-inpainting"){
                flags.filter = C_INPAINTING;
            }else if(filter == "c-lucas-kanade"){
                flags.filter = C_LUCASKANADE;
            }else if (filter == "cl-canny") {
                flags.filter = OCL_CANNY;
            }else if(filter == "cl-hough"){
                flags.filter = OCL_HOUGH;
            }else if(filter == "cl-inpainting"){
                flags.filter = OCL_INPAINTING;
            }else if(filter == "cl-lucas-kanade"){
                flags.filter = OCL_LUCASKANADE;
            }else{
                cout << "Unknown filter '" << filter << "'" << endl;
                exit(1);
            }

        }else if(param == "-ii"){
            /* input image file path */
            flags.image_input = true;
            if (i+1 == argc){
                cerr << "File path missing after '-ii'." << endl;
                exit(1);
            }
            flags.image_path = argv[i+1];
        }else if(param == " "){

        }
    }
}
