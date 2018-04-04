#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <string>
extern "C" {
    #include "c/c-filters.h"
}
#include "opencl/opencl-filters.hpp"

using namespace cv;
using namespace std;

// I fucking love macros
#define LINEAR(y,x) (y)*width+(x)

void overlay(Mat image, string text);
void on_mouse(int event, int x, int y, int flags, void* userdata);

int width;
int height;

Mat img_original;
Mat img_inpainted;
Mat img_masked;
bool* mask_ptr;

bool fill_mode;
int cursor_radius;

/*
   Two modes:
    - Original
    - Filled

    You change between these two modes with 'C'

    In Original mode you can't do anything.
    In Filled mode you can draw a mask with the mouse,
    which will be shown above the image transparently.

    With 'I', we attempt to inpaint with the mask.
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

    overlay(img_original, "Raw");
    overlay(img_inpainted, "Filled");

    img_masked = img_inpainted.clone();

    fill_mode = true;
    cursor_radius = min(width, height) / 20;

    mask_ptr = (bool*) malloc(width * height * sizeof(bool));
    memset(mask_ptr, 0, height * width * sizeof(bool));

    // Loop
    namedWindow("picture", WINDOW_AUTOSIZE);
    setMouseCallback("picture", on_mouse, NULL);

    while (true) {
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
                if (fill_mode) {
                    inpainting((char*) img_inpainted.ptr(), width, height, mask_ptr);
                    memset(mask_ptr, 0, height * width * sizeof(bool));
                    img_masked = img_inpainted.clone();
                }
                break;
            case 'q':
                return 0;
        }
    }


    return 0;
}

bool dragging = false;

// Returns value restricted to range [minimum, maximum]
int clamp(int value, int minimum, int maximum) {
    return value < minimum ? minimum : value > maximum ? maximum : value;
}

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
