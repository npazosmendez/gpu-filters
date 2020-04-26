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
using namespace cv;

Mat read_image(string path) {
    Mat res = cv::imread(path, CV_LOAD_IMAGE_COLOR);
    if(!res.data) {
        ADD_FAILURE() << "Could not read image " << path << "." << endl;
    }
    return res;
}

TEST(ComparisonTest, Canny_C) {
    Mat input_image = read_image("test/data/canny_input.png");
    Mat expected_image = read_image("test/data/canny_expected.png");

    canny((char*)input_image.ptr(), input_image.size().width, input_image.size().height, 100, 70);
    imwrite("test/results/canny_c.png", input_image);
    compare(input_image, expected_image);
}

TEST(ComparisonTest, Canny_CL) {
    Mat input_image = read_image("test/data/canny_input.png");
    Mat expected_image = read_image("test/data/canny_expected.png");

    CL_canny((char*)input_image.ptr(), input_image.size().width, input_image.size().height, 100, 70);
    imwrite("test/results/canny_cl.png", input_image);
    compare(input_image, expected_image);
}

TEST(ComparisonTest, Hough_CL) {
    Mat input_image = read_image("test/data/hough_input.jpeg");
    Mat expected_image = read_image("test/data/hough_expected.png");
    char * counter = (char*)malloc(input_image.size().width * input_image.size().height * 3);

    CL_hough((char*)input_image.ptr(), input_image.size().width, input_image.size().height, 300, 300, counter);
    free(counter);
    imwrite("test/results/hough_cl.png", input_image);
    compare(input_image, expected_image);
}

TEST(ComparisonTest, Hough_C) {
    Mat input_image = read_image("test/data/hough_input.jpeg");
    Mat expected_image = read_image("test/data/hough_expected.png");
    char * counter = (char*)malloc(input_image.size().width * input_image.size().height * 3);

    hough((char*)input_image.ptr(), input_image.size().width, input_image.size().height, 300, 300, counter);
    free(counter);
    imwrite("test/results/hough_c.png", input_image);
    compare(input_image, expected_image);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


