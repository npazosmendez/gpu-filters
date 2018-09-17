#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <stdio.h>
extern "C" {
    #include "c/c-filters.h"
}
#include "opencl/opencl-filters.hpp"

using namespace cv;
using namespace std;

#define LINEAR(y,x) (y)*width+(x)
#define forn(i,n) for(int i=0; i<(n); i++)

void edit_iteration();
void inpaint_iteration();
void paint_mask(Mat img);
void paint_debug(Mat img);
void overlay(Mat image, string text);
void on_mouse(int event, int x, int y, int flags, void* userdata);

int width;
int height;

Mat img_original; // original image
Mat img_inpainted; // latest inpainted image (mutated by algorithm)
Mat img_masked; // image with black mask showed in fill mode
Mat img_step; // image shown when stepping through the algorithm
bool* mask_ptr;
int* debug;

bool inpaint_mode;
bool fill_mode;
int cursor_radius;

void (*picked_init)(int, int, char*, bool*, int*);
bool (*picked_step)(int, int, char*, bool*, int*);


bool quit = false;

/*
   Two modes:
    - Original
    - Filled

    You Change between these two modes with 'C'

    In Original mode you can't do anything.
    In Filled mode you can draw a mask with the mouse,
    which will be shown above the image transparently.

    With 'I', we attempt to Inpaint with the mask.
    Inpainting is done in steps. Every time we press
    'I' again, we do a single step of the algorithm.
    Pressing 'T' will Terminate the inpaint mode by
    running the algorithm until the end.
   */



int main( int argc, char** argv ) {

    // Read Images
    if (argc == 1) {
        cout << "Requires image path as parameter" << endl;
        cout << "Usage: inpainter image_path [optional flags]" << endl;
        return -1;
    }

    img_original = imread(argv[1], CV_LOAD_IMAGE_COLOR);
    if(!img_original.data) {
        cout <<  "Could not open or find the image" << endl;
        return -1;
    }

    picked_init = inpaint_init;
    picked_step = inpaint_step;

    for (int i = 2; i < argc; i++) {
        string param = argv[i];
        if (param == "-cl") {
            /* implementation selection */
            cout << "Using OpenCL implementation..." << endl;
            picked_init = CL_inpaint_init;
            picked_step = CL_inpaint_step;
        } else if (param == "-d") {
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
                return -1;
            }
            selectDevice(device);
        } else if (param == "-h") {
            cout << "Usage:\n";
            cout << "  inpainter image_path [flags]\n";
            cout << "Flags\n";
            cout << "  -cl: Uses OpenCL implementation instead of the C implementation\n";
            cout << "  -d <number>: Picks device to use for the computation\n";
            cout << "\n";

            return 0;
        }
    }

    height = img_original.rows;
    width = img_original.cols;

    // Setup
    img_inpainted = img_original.clone();
    img_masked = img_original.clone();

    overlay(img_original, "Raw");
    overlay(img_masked, "Filled");

    fill_mode = true;
    cursor_radius = min(width, height) / 20;

    mask_ptr = (bool*) malloc(width * height * sizeof(bool));
    memset(mask_ptr, 0, height * width * sizeof(bool));

    debug = (int*) malloc(width * height * sizeof(int));
    memset(debug, 0, width * height * sizeof(int));

    // Loop
    namedWindow("picture", WINDOW_AUTOSIZE);
    setMouseCallback("picture", on_mouse, NULL);

    while (!quit) {
        if (inpaint_mode) {
            inpaint_iteration();
        } else {
            edit_iteration();
        }
    }

    free(mask_ptr);
    free(debug);

    return 0;
}

void edit_iteration() {
    if (fill_mode) {
        imshow("picture", img_masked);
    } else {
        imshow("picture", img_original);
    }

    switch ((char) cv::waitKey(5)) {
        case 'c':
            fill_mode = !fill_mode;
            break;
        case 'i':
            if (fill_mode) inpaint_mode = true;
            break;
        case 'q':
            quit = true;
    }
}

bool first_inpaint_iteration = true;
bool last_inpaint_iteration = false;
bool is_more;

void inpaint_iteration() {
    if (first_inpaint_iteration) {
        picked_init(width, height, (char*) img_inpainted.ptr(), mask_ptr, debug);
        first_inpaint_iteration = false;
    }

    img_step = img_inpainted.clone();
    paint_mask(img_step);
    paint_debug(img_step);
    imshow("picture", img_step);

    switch ((char) cv::waitKey(5)) {
        case 'i':
            is_more = picked_step(width, height, (char*) img_inpainted.ptr(), mask_ptr, debug);
            last_inpaint_iteration = !is_more;
            break;
        case 't':
            while (true) {
                is_more = picked_step(width, height, (char*) img_inpainted.ptr(), mask_ptr, debug);
                if (!is_more) {
                    last_inpaint_iteration = true;
                    break;
                }
            }
            break;
        case 'q':
            quit = true;
    }

    if (last_inpaint_iteration) {
        memset(mask_ptr, 0, height * width * sizeof(bool));
        img_masked = img_inpainted.clone();
        overlay(img_masked, "Filled");
        first_inpaint_iteration = true;
        last_inpaint_iteration = false;
        inpaint_mode = false;
    }
}


// Returns value restricted to range [minimum, maximum]
int clamp(int value, int minimum, int maximum) {
    return value < minimum ? minimum : value > maximum ? maximum : value;
}

bool dragging = false;

void on_mouse(int event, int x, int y, int flags, void* userdata){
    if (fill_mode) {
        if (event == EVENT_LBUTTONDOWN) {
            dragging = true;
        } else if (event == EVENT_LBUTTONUP) {
            dragging = false;
        }

        if (dragging == true) {
            
            for (int i = -cursor_radius; i <= cursor_radius; i++ ) {
                for (int j = -cursor_radius; j <= cursor_radius; j++) {
                    int yi = clamp(y + i, 0, height);
                    int xj = clamp(x + j, 0, width);
                    mask_ptr[LINEAR(yi, xj)] = true;
                    img_masked.at<Vec3b>(Point(xj, yi)) = Vec3b(0, 0, 0);
                }
            }
        }
    }
}

void paint_mask(Mat img) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (mask_ptr[LINEAR(i, j)]) {
                img.at<Vec3b>(Point(j, i)) = Vec3b(0, 0, 0);
            }
        }
    }
}

const Vec3b BLUE = Vec3b(255, 0, 0);
const Vec3b GREEN = Vec3b(0, 255, 0);
const Vec3b RED = Vec3b(0, 0, 255);

void draw_square(Mat img, int i, int j, Vec3b color, int radius = 1) {
    for (int di = -radius; di <= radius; di++) {
        for (int dj = -radius; dj <= radius; dj++) {
            int local_i = clamp(i + di, 0, height);
            int local_j = clamp(j + dj, 0, width);
            img.at<Vec3b>(Point(local_j, local_i)) = color;
        }
    }
}

void paint_debug(Mat img) {
    forn(i, height) forn(j, width) if (debug[LINEAR(i, j)]){
        int val = debug[LINEAR(i, j)];

        if (val == 1) {
            draw_square(img, i, j, BLUE);
        } else if (val == 2) {
            draw_square(img, i, j, RED);
        }
    }
}

void overlay(Mat image, string text) {
    cv::putText(image, 
                text,
                Point(5,30), // Coordinates
                FONT_HERSHEY_TRIPLEX, // Font
                1.0, // Scale. 2.0 = 2x bigger
                Scalar(255,255,255), // Color
                1, // Thickness
                CV_AA); // Anti-alias
}
