#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdlib.h>
#include <iostream>
#include <string>
extern "C" {
    #include "c/c-filters.h"
}
#include "opencl/opencl-filters.hpp"

using namespace cv;
using namespace std;

// TODO
// 1. 'I' should do nothing if there's no mask
// 2. Image should be zoomed to see better
// 3. Start designing how the debug info will be plotted

// I fucking love macros
#define LINEAR(y,x) (y)*width+(x)

void edit_iteration();
void inpaint_iteration();
void paint_mask(Mat img);
void overlay(Mat image, string text);
void on_mouse(int event, int x, int y, int flags, void* userdata);

int width;
int height;

Mat img_original; // original image
Mat img_inpainted; // latest inpainted image (mutated by algorithm)
Mat img_masked; // image with black mask showed in fill mode
Mat img_step; // image shown when stepping through the algorithm
bool* mask_ptr;

bool inpaint_mode;
bool fill_mode;
int cursor_radius;

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
    if (argc != 2) {
        cout << "Requires image path as parameter" << endl;
        return -1;
    }

    img_original = imread(argv[1], CV_LOAD_IMAGE_COLOR);
    if(!img_original.data) {
        cout <<  "Could not open or find the image" << std::endl ;
        return -1;
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

    // Loop
    namedWindow("picture", WINDOW_AUTOSIZE);
    setMouseCallback("picture", on_mouse, NULL);

    while (true) {
        if (inpaint_mode) {
            inpaint_iteration();
        } else {
            edit_iteration();
        }
    }

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
            exit(EXIT_SUCCESS);
    }
}

bool first_inpaint_iteration = true;
bool last_inpaint_iteration = false;
bool is_more;

void inpaint_iteration() {
    if (first_inpaint_iteration) {
        init(width, height, (char*) img_inpainted.ptr(), mask_ptr);
        first_inpaint_iteration = false;
    }

    img_step = img_inpainted.clone();
    paint_mask(img_step);
    imshow("picture", img_step);

    switch ((char) cv::waitKey(5)) {
        case 'i':
            is_more = step(width, height, (char*) img_inpainted.ptr(), mask_ptr);
            last_inpaint_iteration = !is_more;
            break;
        case 't':
            while (true) {
                is_more = step(width, height, (char*) img_inpainted.ptr(), mask_ptr);
                if (!is_more) {
                    last_inpaint_iteration = true;
                    break;
                }
            }
            break;
        case 'q':
            exit(EXIT_SUCCESS);
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
