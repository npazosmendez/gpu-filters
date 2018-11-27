
#include <iostream>
extern "C" {
    #include "../c/cutils.h"
    #include "../c/c-filters.h"
}
#include "opencl/opencl-filters.hpp"

#include <opencv2/highgui/highgui.hpp>
#include <chrono>
#include "utils.hpp"

//using namespace std; // makes some math definition ambiguous in my compiler. Ur ambiguous.
using namespace std;
using namespace cl;
using namespace cv;

int main(int argc, char** argv) {

    Mat input_image = imread("test/data/hough_input.jpeg", CV_LOAD_IMAGE_COLOR);
    Mat expected_image = imread("test/data/hough_expected.png", CV_LOAD_IMAGE_COLOR);
    char * counter = (char*)malloc(input_image.size().width * input_image.size().height * 3);


    string arg = "-cl";
    if (argc > 1) arg= argv[1];

    if (arg == "-cl"){
        CL_hough((char*)input_image.ptr(), input_image.size().width, input_image.size().height, 300, 300, counter);
    }else{
        hough((char*)input_image.ptr(), input_image.size().width, input_image.size().height, 300, 300, counter);
    }

    free(counter);
    compare(input_image, expected_image);
    imwrite("test/data/hough_output.png", input_image);
    return 0;
}