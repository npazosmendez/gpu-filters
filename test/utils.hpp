#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <cmath>
#include <iostream>
using namespace cv;
using namespace std;

#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_RESET   "\x1b[0m"

float MSE(Mat& image1, Mat& image2);
void print_mse(float mse);
void compare(Mat& image1, Mat& image2);