#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <cmath>
#include <iostream>
using namespace cv;
using namespace std;

#include "utils.hpp"
#include <gtest/gtest.h>

float MSE(Mat& image1, Mat& image2){
	Mat diff = image1 - image2;
	int width = image1.size().width;
	int height = image1.size().height;
	float mse = 0;
	for(int i = 0; i < height; i++){
		for(int j = 0; j < width; j++){
			Vec3b pix1 = image1.at<Vec3b>(i,j);
			Vec3b pix2 = image2.at<Vec3b>(i,j);
			float r, g, b;
			r = (float)pow((float)pix1[0] - (float)pix2[0], 2.0);
			g = (float)pow((float)pix1[1] - (float)pix2[1], 2.0);
			b = (float)pow((float)pix1[2] - (float)pix2[2], 2.0);
			mse += r+g+b;
		}
	}
	int size = image1.size().width * image1.size().height * 3;
	return mse / (float)size;
}

void compare(Mat& image1, Mat& image2){
	if (image1.size().height != image2.size().height
		or image1.size().width != image2.size().width){
		FAIL() << "Sizes do not match";
	}
    float mse = MSE(image1, image2);
    ASSERT_LE(mse, 0.1);
}