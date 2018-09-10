#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
extern "C" {
    #include "c/c-filters.h"
}
#include "opencl/opencl-filters.hpp"

using namespace cv;
using namespace std;
using namespace std::chrono;

// Main variables
int width, height;
Mat cameraFrame;

// Benchmarking variables
double time_total = 0;
double time_max = 0;
double time_min = DBL_MAX;
int frames = 0;

// Canny variables
int uthreshold = 20;
int lthreshold = 10;

// Inpainting variables
bool* mask;

// UI misc
void on_mouse(int event, int x, int y, int flags, void* userdata);
void on_trackbar( int, void* ){}
void get_flags(int argc, char** argv);

// Flags
enum filter_t   {C_CANNY,C_HOUGH,C_INPAINTING,C_LUCASKANADE,
    OCL_CANNY,OCL_HOUGH,OCL_INPAINTING,OCL_LUCASKANADE,DEFAULT};

struct flags_t{
    bool file_input = false;
    string file_path;
    filter_t filter = DEFAULT;
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
    createTrackbar( "Higher Thresh", "Canny Parameters", &uthreshold, 100, on_trackbar);
    createTrackbar( "Lower Thresh", "Canny Parameters", &lthreshold, 100, on_trackbar);

    /* Inpainting parameters arbitrary set */
    mask = (bool*) malloc(width * height * sizeof(bool));

    /* Init OpenCL */
    initCL();

    while (true) {
        if(!stream.read(cameraFrame)){ // get camera frame
            stream.set(CV_CAP_PROP_POS_MSEC,0); // reset if ended (for file streams)
            continue;
        }

        char *ptr = (char*)cameraFrame.ptr();

        steady_clock::time_point t1 = steady_clock::now();

        switch (flags.filter) {
            case C_CANNY:
            canny(ptr,width,height,uthreshold,lthreshold);
            break;
            case OCL_CANNY:
            CL_canny(ptr, width, height, uthreshold, lthreshold);
            break;
            case C_INPAINTING:
            inpaint_generate_arbitrary_mask(mask, width, height);
            inpainting(ptr, width, height, mask, NULL);
            break;
            default:
            break;
        }

        steady_clock::time_point t2 = steady_clock::now();
        // A duration object also has another template parameter that defines
        // what unit to use, it it's ommited, it's in seconds
        duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
        double milliseconds = time_span.count() * 1000.0;

        frames++;
        time_min = min(time_min, milliseconds);
        time_max = max(time_max, milliseconds);
        time_total = time_total + milliseconds;

        imshow("gpu-filters", cameraFrame); // show frame

        if ((char)cv::waitKey(5) == 'q') break;
    }

    cout << "\n"                                                 \
         << "Metrics (" << frames << " samples)\n"               \
         << setprecision(8) << fixed                             \
         << " AVG = " << time_total / double(frames) << " ms\n"  \
         << " MAX = " << time_max << " ms\n"                     \
         << " MIN = " << time_min << " ms\n"                     \
         << "\n";

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

        if (param == "-h") {
            cout << "Parameters\n";
            cout << "\n";

            cout << "-f <filter-name>\n";
            cout << "  Specifies the filter that will be applied to the video stream\n";
            cout << "  Possible values:\n";
            cout << "   C implementations: c-canny, c-hough, c-inpainting, c-lucas-kanade\n";
            cout << "   OpenCL implementations: cl-canny, cl-hough, cl-inpainting, cl-lucas-kanade\n";
            cout << "\n";

            cout << "-i <image-path>\n";
            cout << "  Specifies a file path for something Nico knows but I don't\n";
            cout << "\n";

            cout << "-d <device-number>\n";
            cout << "  Specifies what OpenCL device will be used for processing on the first platform \n";
            cout << "\n";

            exit(0);

        } else if (param == "-i") {
            /* input file path */
            flags.file_input = true;
            if (i+1 == argc){
                cerr << "File path missing after '-i'." << endl;
                exit(1);
            }
            flags.file_path = argv[i+1];
        } else if (param == "-f") {
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

        } else if (param == "-d"){
            /* device selection */
            if (i+1 == argc) {
                cerr << "Filter name missing after '-d'." << endl;
                exit(1);
            }
            int device;
            try {
                device = stoi(argv[i+1]);
            } catch (int e) {
                cerr << "Couldn't convert '-d' parameter to number" << endl;
                exit(1);
            }
            selectDevice(device);
        } else if (param == " "){

        }
    }
}
