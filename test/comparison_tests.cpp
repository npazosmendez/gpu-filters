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

#include <gtest/gtest.h>

using namespace std;
using namespace cl;
using namespace cv;

TEST(ComparisonTest, Canny_C) { 
    Mat input_image = imread("test/data/canny_input.png", CV_LOAD_IMAGE_COLOR);
    Mat expected_image = imread("test/data/canny_expected.png", CV_LOAD_IMAGE_COLOR);

    canny((char*)input_image.ptr(), input_image.size().width, input_image.size().height, 100, 70);
    compare(input_image, expected_image);
}

TEST(ComparisonTest, Canny_CL) { 
    Mat input_image = imread("test/data/canny_input.png", CV_LOAD_IMAGE_COLOR);
    Mat expected_image = imread("test/data/canny_expected.png", CV_LOAD_IMAGE_COLOR);

    CL_canny((char*)input_image.ptr(), input_image.size().width, input_image.size().height, 100, 70);
    compare(input_image, expected_image);
}

TEST(ComparisonTest, Hough_CL) { 
    Mat input_image = imread("test/data/hough_input.png", CV_LOAD_IMAGE_COLOR);
    Mat expected_image = imread("test/data/hough_expected.png", CV_LOAD_IMAGE_COLOR);
    char * counter = (char*)malloc(input_image.size().width * input_image.size().height * 3);

    CL_hough((char*)input_image.ptr(), input_image.size().width, input_image.size().height, 300, 300, counter);
    free(counter);
    compare(input_image, expected_image);
}

TEST(ComparisonTest, Hough_C) { 
    Mat input_image = imread("test/data/hough_input.png", CV_LOAD_IMAGE_COLOR);
    Mat expected_image = imread("test/data/hough_expected.png", CV_LOAD_IMAGE_COLOR);
    char * counter = (char*)malloc(input_image.size().width * input_image.size().height * 3);

    hough((char*)input_image.ptr(), input_image.size().width, input_image.size().height, 300, 300, counter);
    free(counter);
    compare(input_image, expected_image);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


