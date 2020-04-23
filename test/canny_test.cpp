
#include <iostream>
extern "C" {
    #include "c/cutils.h"
    #include <c-filters.h>
}
#include <opencl-filters.hpp>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <chrono>
#include "utils.hpp"

//using namespace std; // makes some math definition ambiguous in my compiler. Ur ambiguous.
using namespace std;
using namespace cl;
using namespace cv;

int main(int argc, char** argv) {

    Mat input_image = imread("test/data/canny_input.png", CV_LOAD_IMAGE_COLOR);
    Mat expected_image = imread("test/data/canny_expected.png", CV_LOAD_IMAGE_COLOR);

    string arg = "-cl";
    if (argc > 1) arg= argv[1];

    if (arg == "-cl"){
    	CL_canny((char*)input_image.ptr(), input_image .size().width, input_image   .size().height, 100, 70);
    }else{
    	canny((char*)input_image.ptr(), input_image .size().width, input_image   .size().height, 100, 70);
    }

    compare(input_image, expected_image);
    imwrite("test/data/canny_output.png", input_image);
    return 0;
}